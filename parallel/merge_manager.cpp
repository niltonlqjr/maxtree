#include <string>
#include <thread>

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

uint32_t glines, gcolumns;



bool print_only_trees;
bool verbose;

bool running;

static const int nth = 2;

TWorkerIdx idx_at_manager=0;

scheduler_of_workers<worker*> pool_of_workers;

bag_of_tasks<boundary_tree_task *> bound_trees;
bag_of_tasks<merge_btrees_task *> merge_bag;
bag_of_tasks<input_tile_task* > input_tiles;

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
    glines = std::stoi(configs->at("glines"));
    gcolumns = std::stoi(configs->at("gcolumns"));

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
    while(bound_trees.is_running() || !bound_trees.get_num_task() > 1){

        bool got = bound_trees.get_task(btt);
        if(got){
            if(btt->nb_distance.first == 0){
                merge_dir = MERGE_VERTICAL_BORDER;
            }else if(btt->nb_distance.second == 0){
                merge_dir = MERGE_HORIZONTAL_BORDER;
            }else{
                std::cerr << "merge distance invalid: " << int_pair_to_string(btt->nb_distance) << "\n";
                exit(EXIT_FAILURE);
            }
            // s = "got tree: " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + 
            //     " with distance: "+ int_pair_to_string(btt->nb_distance) +"\n";
            // std::cout << s;
            if(merge_dir == MERGE_VERTICAL_BORDER){
                if(btt->bt->grid_j % int_pow(2,btt->nb_distance.second) == 0){
                    nb_direction = NB_AT_RIGHT;
                }else{
                    nb_direction = NB_AT_LEFT;
                }
            }else if(merge_dir == MERGE_HORIZONTAL_BORDER){
                if(btt->bt->grid_i % int_pow(2,btt->nb_distance.first) == 0){
                    nb_direction = NB_AT_BOTTOM;
                }else{
                    nb_direction = NB_AT_TOP;
                }
            }
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
                try{
                    auto got_n = bound_trees.get_task_by_field<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    
                    if(!got_n){
                        bound_trees.insert_task(btt);
                        // std::string s = "no neighbor" + std::to_string(btt->index.first) + "," + std::to_string(btt->index.second) + "\n";
                        // std::cout << s;
                    }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){

                        if((merge_dir == MERGE_VERTICAL_BORDER) && (btt->bt->grid_j > n->bt->grid_j) ||
                           (merge_dir == MERGE_HORIZONTAL_BORDER) && (btt->bt->grid_i > n->bt->grid_i)){
                            if(verbose){
                                std::string s = "swap: " + int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + "\n";
                                std::cout << s;
                            }
                            aux = btt;
                            btt = n;
                            n = aux;
                        }
                        if(merge_dir == MERGE_HORIZONTAL_BORDER&& btt->bt->border_elements->at(BOTTOM_BORDER)->size() != n->bt->border_elements->at(TOP_BORDER)->size()){
                            std::string s = int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + " borders differs in size\n";
                            s += "distances:" + int_pair_to_string(btt->nb_distance) + " and " + int_pair_to_string(n->nb_distance) + "\n";
                            std::cout << s;
                        }

                        // s = "creating task with btt " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + "   ";
                        // s += "n: "  + std::to_string(n->bt->grid_i) + "," + std::to_string(n->bt->grid_j) + "distance ";
                        // s += int_pair_to_string(btt->nb_distance) +"\n";
                        // // std::cout << s;
                        auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                        merge_bag.insert_task(new_merge_task);
                    }else{
                        if(verbose){
                            std::string s = "invalid distance:" + int_pair_to_string(btt->index) + " ->" + int_pair_to_string(btt->nb_distance) + "\n";
                            s+="invalid distance:" + int_pair_to_string(n->index) + " ->" + int_pair_to_string(n->nb_distance) + "\n=======\n";
                            std::cout << s;
                        }
                        bound_trees.insert_task(n);
                        bound_trees.insert_task(btt);
                    }
                }
                catch(std::runtime_error &e){
                    std::cerr << e.what();
                    // s = "pair of " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + " not found ";
                    // s += std::to_string(idx_nb.first) + "," + std::to_string(idx_nb.second) + "\n";
                    // std::cout << s;
                    bound_trees.insert_task(btt);
                }catch(std::out_of_range &r){
                    std::cerr << "try to access an out of range element\n";
                }
            }else if (inside_rectangle(btt->index, GRID_DIMS)){ // the neighbor of btt is not inside the grid (it does not exist, so go to next merge)
                if(btt->nb_distance.first == 0){
                    if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " line distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.second *= 2;
                    }else{ // there is no more neighbor on this line to merge, so this line must be merged with the other lines
                        
                        btt->nb_distance.first = 1; 
                        btt->nb_distance.second = 0;
                    }
                }else if(btt->nb_distance.second == 0){
                    if(btt->nb_distance.first < GRID_DIMS.first){
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " column distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.first *= 2;
                    }
                }
                bound_trees.insert_task(btt);   
            }
        }
    }
    if(verbose) std::cout << "end worker search pair\n";
}

// merge_btrees_task find_task(worker *w){
//     while(running){
//         merge_bag.get_task();
//     }
// }

std::pair<uint32_t, uint32_t> next_tile(std::pair<uint32_t, uint32_t> p){
    std::pair<uint32_t, uint32_t> newp;
    newp.second = (p.second + 1) % GRID_DIMS.second;
    newp.first = p.first + (newp.second != p.second+1);
    return newp;
}

void manager_recv(zmq::socket_t &sock){

    // todos os trabalhadores devem procurar um balanceamento na equação 1.
    //      trabalho_realizado = poder_de_processamento * 1/peso_das_tarefas_realizadas (1)
    // sendo assim, todas solicitações vão procurar responder com o trabalhador que tem
    // o menor coeficiente de trabalho_realizado dentre os disponíveis.
    // é uma estratégia gulosa.

    zmq::message_t request;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::unordered_map<TWorkerIdx, worker *> busy_workers;
    TWorkerIdx current_idx;

    while (running){
        std::cout << "------>new iteration\nwaiting message...";
        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        
        std::string rec_msg;
        rec_msg = request.to_string();
        // rec_msg.resize(request.size());
        // memcpy(rec_msg.data(), request.data(), request.size());
        // std::cout << "receieved:" << rec << "\n";
        // std::cout << "receieved: --> ";
        // for(char c: rec_msg){
        //     std::cout << " " << (int) c;
        // }
        // std::cout << " <--\n";
        message recv_msg = hps::from_string<message>(rec_msg);
        std::cout << "message type" << recv_msg.type << "\n";
        if(recv_msg.type == MSG_REGISTRY){
            worker w_rec = hps::from_string<worker>(recv_msg.content);
            std::cout << "Registry" << "\n";
            // w_rec.print();
            current_idx=idx_at_manager++;
            w_rec.update_index(current_idx);
            worker *w_at_manager = new worker(w_rec);
            pool_of_workers.insert_worker(w_at_manager);
            // std::cout << "========================== registered workers: ==========================\n";
            // for(size_t i=0; i < pool_of_workers.size(); i++){
            //     worker *w = pool_of_workers.at(i);
            //     // w->print();
            // }
            // std::cout << "==========================  ==========================\n";
            
            zmq::message_t msg_new_id(sizeof(TWorkerIdx));
            memcpy(msg_new_id.data(), &current_idx, sizeof(TWorkerIdx));
            
            auto reply_return = sock.send(msg_new_id, zmq::send_flags::none);

        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            std::cout << "tree" << "\n";
            boundary_tree bt = hps::from_string<boundary_tree>(recv_msg.content);
            std::cout << "tree ok\n";
            // std::cout << "-------------------------- Boundary Tree ----------------------------\n";
            
            boundary_tree *ptr_tree = new boundary_tree(bt);
            auto dist_ini = std::make_pair<uint32_t,uint32_t>(0,1);
            // ptr_tree->print_tree();
            bound_trees.insert_task(new boundary_tree_task(ptr_tree,dist_ini));
            auto reply_return = sock.send(zmq::str_buffer("") ,zmq::send_flags::none);


        }else if(recv_msg.type == MSG_GET_TASK){
            
            std::cout << "get task\n";
            TWorkerIdx worker_idx = hps::from_string<TWorkerIdx>(recv_msg.content);
            message reply;
            input_tile_task *task;
            std::pair<uint32_t,uint32_t> idx_reply;
            reply.sender = self_address;
            if(!input_tiles.empty()){
                if(input_tiles.get_task(task)){
                    std::cout << "tile task bag... ";
                    idx_reply = std::make_pair(task->i, task->j);
                    std::cout << "got tile task\n";
                    delete task;
                }else{
                    idx_reply = GRID_DIMS;
                }
                reply.type = MSG_TILE_IDX;
                reply.content = hps::to_string<std::pair<uint32_t,uint32_t>>(idx_reply);
                reply.size = reply.content.size();
                
            }else if(!merge_bag.empty()){
                reply.type = MSG_MERGE_BOUNDARY_TREE;
                merge_btrees_task *btt;
                if(merge_bag.get_task(btt)){
                    std::cout << "merge task bag... ";
                    reply.content = hps::to_string(*btt);
                    reply.size = reply.content.size();
                    // auto x = btt->execute();
                    // x->print_tree();
                    std::cout << "got merge task\n";
                }
            }else{
                reply.type = MSG_NULL;
                reply.content = "";
                reply.size = 0;
            }
            std::string reply_s = hps::to_string(reply);
            zmq::message_t msg_reply(reply_s);
            sock.send(msg_reply, zmq::send_flags::none);
        }
        std::cout << "end iteration<--------\n";
    }
}

void fill_input_bag(){
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::pair<uint32_t,uint32_t> grid_idx = current_tile;

    while(inside_rectangle(grid_idx,GRID_DIMS)){
        input_tiles.insert_task(new input_tile_task(grid_idx));
        grid_idx = next_tile(current_tile);               
        current_tile = grid_idx;
    }
}


int main(int argc, char *argv[]){
    
    std::string port, protocol;

    read_config(argv[1], port, protocol);
    GRID_DIMS = std::make_pair(glines,gcolumns);
    //  Prepare contexts and sockets
    
    zmq::context_t context_rec(nth);
    zmq::context_t context_reg(nth);
    zmq::socket_t  receiver_sock(context_rec, zmq::socket_type::pull);
    zmq::socket_t  sock(context_reg, zmq::socket_type::rep);
   
    self_address = protocol+"://*:"+port;
    std::string address = self_address;
    sock.bind(address);
    
    running=true;
    std::thread fill(fill_input_bag);
    input_tiles.start();
    std::thread receiver(manager_recv, std::ref(sock));
    bound_trees.start();
    std::thread pair_maker(search_pair);
    merge_bag.start();


    fill.join();
    pair_maker.join();
    receiver.join();
    
}