#include <string>
#include <thread>
#include <cmath>

#include <zmq.hpp>

#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
#include "boundary_tree.hpp"
#include "message.hpp"
#include "src/hps.h"

using namespace vips;

std::string self_address;

uint32_t G_glines, G_gcolumns;
uint64_t G_total_tiles, G_updates_sent, G_total_workers, G_finished_workers, G_num_merges;

bool print_only_trees;
bool verbose;

bool running;

static const int nth = 2;

TWorkerIdx G_idx_at_manager=1; //index count of workers. Index 0 is for manager

boundary_tree *G_reply_bt=nullptr;
boundary_tree_task *G_reply_btt=nullptr;

scheduler_of_workers<worker*> G_pool_of_workers;

bag_of_tasks<input_tile_task* > G_input_tiles;
bag_of_tasks<boundary_tree_task *> G_bound_trees;
prio_bag_of_tasks<merge_btrees_task *> G_merge_bag;

uint64_t G_total_merges;

// given that unordered_map needs a hash function for pair and there are few grids, a map is enought
std::map <std::pair<uint32_t,uint32_t>, worker *> G_grid_id_worker;  

std::unordered_map<std::string, bool> G_got_full_btree;

void read_config(char conf_name[], std::string &port, std::string &protocol);

bool reply_to(std::string ip, worker *w){
    zmq::context_t context(1);
    zmq::socket_t rep(context, zmq::socket_type::push);
    rep.connect(ip);
    std::string s_newid = hps::to_string<TWorkerIdx>(w->get_index());
    zmq::message_t msg_newid(s_newid);
    std::optional<size_t> send_result = rep.send(msg_newid,zmq::send_flags::none);
    if(send_result.has_value()){
        rep.disconnect(ip);
        return true;
    }
    return false;
}

void read_config(char conf_name[], std::string &port, std::string &protocol){

    auto configs = parse_config(conf_name);

    if(configs->find("glines") == configs->end() || configs->find("gcolumns") == configs->end()){
        std::cout << "you must specify the the image division on config file:" << conf_name <<"\n";
        std::cout << "example:\n";
        std::cout << "glines=8 #divide image in 8 vertical tiles\n";
        std::cout << "gcolumns=6 #divide image in 6 vertical tiles\n";
        exit(EX_CONFIG);
    }
    G_glines = std::stoi(configs->at("glines"));
    G_gcolumns = std::stoi(configs->at("gcolumns"));

    port = get_field(configs, "port", DEFAULT_PORT);
    protocol = get_field(configs, "protocol", "tcp");
}

void search_pair(){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    while(G_merge_bag.is_running() || G_bound_trees.get_num_task() > 1){

        bool got = G_bound_trees.get_task(btt);
        if(got){
            // if(btt->nb_distance.first == 0){
            //     merge_dir = MERGE_VERTICAL_BORDER;
            // }else if(btt->nb_distance.second == 0){
            //     merge_dir = MERGE_HORIZONTAL_BORDER;
            // }else{
            //     std::cerr << "merge distance invalid: " << int_pair_to_string(btt->nb_distance) << "\n";
            //     std::cerr << "index: ";
            //     exit(EXIT_FAILURE);
            // }
            merge_dir = btt->define_merge_direction();

            // if(merge_dir == MERGE_VERTICAL_BORDER){
            //     if(btt->bt->grid_j % int_pow(2,btt->nb_distance.second) == 0){
            //         nb_direction = NB_AT_RIGHT;
            //     }else{
            //         nb_direction = NB_AT_LEFT;
            //     }
            // }else if(merge_dir == MERGE_HORIZONTAL_BORDER){
            //     if(btt->bt->grid_i % int_pow(2,btt->nb_distance.first) == 0){
            //         nb_direction = NB_AT_BOTTOM;
            //     }else{
            //         nb_direction = NB_AT_TOP;
            //     }
            // }

            nb_direction = btt->define_nb_direction(merge_dir);
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
                try{
                    auto got_n = G_bound_trees.get_task_by_field<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    
                    if(!got_n){
                        G_bound_trees.insert_task(btt);
                    }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){

                        // if((merge_dir == MERGE_VERTICAL_BORDER) && (btt->bt->grid_j > n->bt->grid_j) ||
                        //    (merge_dir == MERGE_HORIZONTAL_BORDER) && (btt->bt->grid_i > n->bt->grid_i)){
                        //     aux = btt;
                        //     btt = n;
                        //     n = aux;
                        // }
                        auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                        G_merge_bag.insert_task(new_merge_task);
                    }else{
                        G_bound_trees.insert_task(n);
                        G_bound_trees.insert_task(btt);
                    }
                }
                catch(std::runtime_error &e){
                    std::cerr << e.what();
                    G_bound_trees.insert_task(btt);
                }catch(std::out_of_range &r){
                    std::cerr << "try to access an out of range element\n";
                }
            }else if (inside_rectangle(btt->index, GRID_DIMS)){ // the neighbor of btt is not inside the grid (it does not exist, so go to next merge)
                if(btt->nb_distance.first == 0){
                    if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " line distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            // std::cout << s;
                        }
                        btt->nb_distance.second *= 2;
                    }else{ // there is no more neighbor on this line to merge, so this line must be merged with the other lines
                        
                        btt->nb_distance.first = 1; 
                        btt->nb_distance.second = 0;
                    }
                }else if(btt->nb_distance.second == 0){
                    if(btt->nb_distance.first < GRID_DIMS.first){
                        // if(verbose){
                        //     std::string s = int_pair_to_string(btt->index) + " column distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                        //     std::cout << s;
                        // }
                        btt->nb_distance.first *= 2;
                    }
                }
                G_bound_trees.insert_task(btt);   
            }
        }
    }
    // if(verbose) std::cout << "end worker search pair\n";
    // std::cout << "end worker search pair\n";
}


std::pair<uint32_t, uint32_t> next_tile(std::pair<uint32_t, uint32_t> p){
    std::pair<uint32_t, uint32_t> newp;
    newp.second = (p.second + 1) % GRID_DIMS.second;
    newp.first = p.first + (newp.second != p.second+1);
    return newp;
}

void registry_worker(message &recv_msg, zmq::socket_t &sock){
    worker w_rec = hps::from_string<worker>(recv_msg.content);
    TWorkerIdx current_idx=G_idx_at_manager++;
    w_rec.update_index(current_idx);
    worker *w_at_manager = new worker(w_rec);
    G_pool_of_workers.insert_worker(w_at_manager);
    G_got_full_btree[w_at_manager->get_self_address()] = false;
    zmq::message_t msg_new_id(sizeof(TWorkerIdx));
    memcpy(msg_new_id.data(), &current_idx, sizeof(TWorkerIdx));
    auto reply_return = sock.send(msg_new_id, zmq::send_flags::none);
    G_total_workers++;
}

void recv_boundary_tree(message &recv_msg, zmq::socket_t &sock){
    boundary_tree_task btt = hps::from_string<boundary_tree_task>(recv_msg.content);
    boundary_tree_task *new_bttask = new boundary_tree_task(btt);
    G_bound_trees.insert_task(new_bttask);
    message reply = message(0);
    std::string s_reply = hps::to_string<message>(reply);
    auto reply_return = sock.send(zmq::message_t(s_reply), zmq::send_flags::none);
}

void prepare_tile(message &reply){
    input_tile_task *task;
    std::pair<uint32_t,uint32_t> idx_reply;
    if(G_input_tiles.get_task(task)){
        idx_reply = std::make_pair(task->i, task->j);
        delete task;
    }else{
        idx_reply = GRID_DIMS;
    }
    reply.type = MSG_TILE_IDX;
    reply.content = hps::to_string<std::pair<uint32_t,uint32_t>>(idx_reply);
}

void schedule_merge(){
    return;
}

/*
reply.type = MSG_UPDATE_BOUNDARY_TREE;
if(!G_got_full_btree[recv_msg.content]){
    if(G_final_task == nullptr){
        if(!G_bound_trees.get_task(G_final_task)){
            std::cerr << "Error trying to get final boundary tree.\n";
            exit(EX_SOFTWARE);
        }
        G_final_bt = G_final_task->bt;
        // G_bound_trees.notify_end();
    }
    reply.content = hps::to_string<boundary_tree>(*G_final_bt);
    G_got_full_btree[recv_msg.content]=true;
    
}else{
    reply.content="";
}
G_updates_sent++;
*/

void prepare_final_tree(message &reply, std::string worker){
    reply.type = MSG_UPDATE_BOUNDARY_TREE;
    if(!G_got_full_btree[worker]){
        if(G_reply_btt == nullptr){
            G_bound_trees.get_task(G_reply_btt);
            G_reply_bt = G_reply_btt->bt;
            std::cout << "final tree got "<< G_reply_bt->index_to_string() << "\n\n" ;
        }
        reply.content = hps::to_string<boundary_tree>(*G_reply_bt);
        // G_reply_bt->print_tree();
        G_got_full_btree[worker]=true;
    }else{
        reply.content="";
    }
}

void process_task_request(message &recv_msg, zmq::socket_t &sock){
    TWorkerIdx worker_idx = hps::from_string<TWorkerIdx>(recv_msg.content);
    message reply;
    reply.sender = 0;
    if(!G_input_tiles.empty()){ // there is tiles of image to read and calculate maxtree yet
        prepare_tile(reply);
    // }else if(!G_merge_bag.empty()){ // there is merge to be done
    }else if(!G_merge_bag.empty()){
        reply.type = MSG_MERGE_BOUNDARY_TREE;
        merge_btrees_task *btt;
        if(G_merge_bag.get_task(btt)){
            // btt->bt1->print_idx(" ---- ");
            // btt->bt2->print_idx();
            reply.content = hps::to_string(*btt);
            // reply.size = reply.content.size();
        }

    }else if(!G_merge_bag.is_running()){ // final boundary tree is ready
        prepare_final_tree(reply,recv_msg.content);
        G_updates_sent++;
    }else if(G_updates_sent > G_total_tiles){
        reply.type = MSG_COMMAND;
        reply.content = "END";
        G_finished_workers++;
        // reply.size = 0;
    }
    reply.size = reply.content.size();
    std::string reply_s = hps::to_string(reply);
    zmq::message_t msg_reply(reply_s);
    sock.send(msg_reply, zmq::send_flags::none);
}

void manager_recv(zmq::socket_t &sock){

    // todos os trabalhadores devem procurar um balanceamento na equação 1.
    //      trabalho_realizado = poder_de_processamento * 1/peso_das_tarefas_realizadas (1)
    // sendo assim, todas solicitações vão procurar responder com o trabalhador que tem
    // o menor coeficiente de trabalho_realizado dentre os disponíveis.
    // é uma estratégia gulosa.

    zmq::message_t request;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::unordered_map<TWorkerIdx, bool > busy_workers;
    
    G_num_merges = 0;
    uint64_t calculated_tiles = 0;
    // TWorkerIdx current_idx;
    while(G_updates_sent < G_total_tiles){
    // while(num_merges < G_total_merges || !busy_workers.empty()){
        // std::cout << "updates: " << G_updates_sent << "\n";

        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        std::string rec_msg;
        rec_msg = request.to_string();
        message recv_msg = hps::from_string<message>(rec_msg);
        // std::cout << "type:"<< recv_msg.type << " -> " << NamesMessageType[recv_msg.type] << "\n";
        if(recv_msg.type == MSG_REGISTRY){
            registry_worker(recv_msg, sock);
        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            recv_boundary_tree(recv_msg, sock);
        }else if(recv_msg.type == MSG_SEND_MERGED_TREE){
            recv_boundary_tree(recv_msg, sock);
            G_num_merges++;
            // std::cout << "merge "<< num_merges << " of " << G_total_merges <<" ends\n";
            busy_workers.erase(recv_msg.sender);
            if(G_num_merges >= G_total_merges){
                G_merge_bag.notify_end();
                std::cout << "merge bag end\n";
            }
        }else if(recv_msg.type == MSG_GET_TASK){
            process_task_request(recv_msg, sock);
            busy_workers[recv_msg.sender] = true;

        }
        // std::cout << "end iteration<--------\n";
    }
    G_bound_trees.notify_end();
    // std::cout << "\n==============\nend manager_recv\n";
    
    
}

void fill_input_bag(){
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::pair<uint32_t,uint32_t> grid_idx = current_tile;

    while(inside_rectangle(grid_idx,GRID_DIMS)){
        G_input_tiles.insert_task(new input_tile_task(grid_idx));
        grid_idx = next_tile(current_tile);               
        current_tile = grid_idx;
    }
}

void finish_workers(zmq::socket_t &sock){
    std::cout << "finish_workers\n";
    zmq::message_t request;
    message reply;
    while(G_finished_workers < G_total_workers){
        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        std::string rec_msg;
        rec_msg = request.to_string();
        message recv_msg = hps::from_string<message>(rec_msg);
        reply.type = MSG_COMMAND;
        reply.content = "END";
        // reply.size = 0;
        reply.size = reply.content.size();
        std::string reply_s = hps::to_string(reply);
        zmq::message_t msg_reply(reply_s);
        sock.send(msg_reply, zmq::send_flags::none);
        G_finished_workers++;
    }
}


int main(int argc, char *argv[]){
    
    std::string port, protocol;

    read_config(argv[1], port, protocol);
    GRID_DIMS = std::make_pair(G_glines,G_gcolumns);
    //  Prepare contexts and sockets
    
    
    G_total_merges = G_glines * (G_gcolumns - 1) + G_glines - 1;
    G_total_tiles = G_glines * G_gcolumns;
    G_updates_sent = 0;
    G_total_workers = 0;
    G_finished_workers = 0;
    zmq::context_t context_rec(nth);
    zmq::context_t context_reg(nth);
    zmq::socket_t  receiver_sock(context_rec, zmq::socket_type::pull);
    zmq::socket_t  sock(context_reg, zmq::socket_type::rep);
    
    self_address = protocol+"://*:"+port;
    std::string address = self_address;
    sock.bind(address);
    
    std::cout << "running at port " << port << "\n";

    
    running=true;
    G_input_tiles.start();
    G_bound_trees.start();
    G_merge_bag.start();
 
    std::thread fill(fill_input_bag);
    std::thread receiver(manager_recv, std::ref(sock));
    std::thread pair_maker(search_pair);

    fill.join();
    pair_maker.join();
    receiver.join();

    finish_workers(sock);
    
}