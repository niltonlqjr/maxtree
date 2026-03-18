#include <string>
#include <thread>
#include <cmath>

#include <zmq.hpp>

#include "bag_of_task.hpp"
#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
#include "boundary_tree.hpp"
#include "message.hpp"
#include "src/hps.h"




using namespace vips;


void read_config(char conf_name[], std::string &port, std::string &protocol);
void search_pair_naive();
void search_pair();

std::pair<uint32_t, uint32_t> next_tile(std::pair<uint32_t, uint32_t> p);

merge_btrees_task *get_task_naive(TWorkerIdx idx);
merge_btrees_task *get_task_balanced(TWorkerIdx idx);
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
// ordered_scheduler_of_workers <worker*, worker_lesser_than> G_pool_of_workers;

bag_of_tasks<input_tile_task* > G_input_tiles;
bag_of_tasks<boundary_tree_task *> G_bound_trees;

bag_of_tasks<merge_btrees_task *> G_merge_bag;
// prio_bag_of_tasks<merge_btrees_task *> G_merge_bag;
// ordered_bag_of_tasks<merge_btrees_task *, mbt_lesser_than> G_merge_bag;

uint64_t G_total_merges;

// given that unordered_map needs a hash function for pair and there are few grids, a map is enought
std::map <std::pair<uint32_t,uint32_t>, worker *> G_grid_id_worker;  

std::unordered_map<std::string, bool> G_got_full_btree;


std::unordered_map<uint64_t, std::vector<boundary_tree_task*>> G_waiting;
std::mutex G_waiting_lock;


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

boundary_tree_task *get_neighbor_bound_task(std::vector<boundary_tree_task*> &v, boundary_tree_task* t){
    boundary_tree_task *r=nullptr;
    for(size_t i=0; i<v.size(); i++){
        if(v[i]->can_merge_with(t)){
            v.erase(v.begin()+i);
            return v[i];
        }
    }
    return nullptr;
}

void search_pair_naive(){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    uint64_t self_int_idx, nb_int_idx;
    while(G_merge_bag.is_running() || G_bound_trees.size() > 1){
        // std::cout << "search pair naive\n";
        bool got = G_bound_trees.get_task(btt);
        if(got){
            self_int_idx = index_of(btt->index, GRID_DIMS);
            merge_dir = btt->define_merge_direction();
            nb_direction = btt->define_nb_direction(merge_dir);
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
                try{
                    auto got_n = G_bound_trees.get_task_by_function<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    if(!got_n){
                        G_bound_trees.insert_task(btt);

                    }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){
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
                        }
                        btt->nb_distance.second *= 2;
                    }else{ // there is no more neighbor on this line to merge, so this line must be merged with the other lines
                        btt->nb_distance.first = 1; 
                        btt->nb_distance.second = 0;
                    }
                }else if(btt->nb_distance.second == 0){
                    if(btt->nb_distance.first < GRID_DIMS.first){
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

void search_pair(){
    boundary_tree_task *btt, *nb, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    uint64_t self_int_idx, nb_int_idx;
    while(G_merge_bag.is_running() || G_bound_trees.size() > 1){

        bool got = G_bound_trees.get_task(btt);
        if(got){
            self_int_idx = index_of(btt->index, GRID_DIMS);
            merge_dir = btt->define_merge_direction();
            nb_direction = btt->define_nb_direction(merge_dir);
            idx_nb = btt->neighbor_idx(nb_direction);
            nb_int_idx = index_of(idx_nb,GRID_DIMS);
            
            G_waiting_lock.lock();
            if(G_waiting.find(self_int_idx) == G_waiting.end()){
                G_waiting[self_int_idx] = std::vector<boundary_tree_task*>();
            }
            if(G_waiting.find(nb_int_idx) == G_waiting.end()){
                G_waiting[nb_int_idx] = std::vector<boundary_tree_task*>();
            }
            G_waiting_lock.unlock();

            if(inside_rectangle(idx_nb, GRID_DIMS)){
                nb = get_neighbor_bound_task(G_waiting[self_int_idx], btt);
                if(!nb){ // if the neighbor was not found, is needed to add it 
                    G_waiting_lock.lock();
                    G_waiting[nb_int_idx].push_back(btt); 
                    G_waiting_lock.unlock();
                }else if (nb->can_merge_with(btt)){ // the neighbor was found and can merge with btt
                    merge_btrees_task *mbt = new merge_btrees_task(btt, nb);
                    G_merge_bag.insert_task(mbt);
                }else{ // the neighbor was found and cannot merge
                    G_bound_trees.insert_task(nb);
                    G_bound_trees.insert_task(btt);
                }
            }else{
                btt->nb_distance.first  *= 2;
                btt->nb_distance.second *= 2;
                if(btt->nb_distance.second > GRID_DIMS.second){
                    btt->nb_distance.first  *= 1;
                    btt->nb_distance.second *= 0;
                }else if(btt->nb_distance.first > GRID_DIMS.first){
                    break;
                }
                G_bound_trees.insert_task(btt);
            }
        }
    }

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
    worker *w_at_manager = new worker(w_rec);
    w_at_manager->update_index(current_idx);
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
    std::cout << "tile: " << int_pair_to_string(idx_reply) << "sent \n";
    reply.type = MSG_TILE_IDX;
    reply.content = hps::to_string<std::pair<uint32_t,uint32_t>>(idx_reply);
}

TWorkerIdx get_worker_idx(worker *w){
    return w->get_index();
}

std::pair<uint64_t, uint64_t> index_int_pair(merge_btrees_task *t){
    std::pair<uint32_t, uint32_t> pidx_bt1 = std::make_pair(t->bt1->grid_i, t->bt1->grid_j);
    std::pair<uint32_t, uint32_t> pidx_bt2 = std::make_pair(t->bt2->grid_i, t->bt2->grid_j);
    
    uint64_t idx_bt1 = index_of(pidx_bt1, GRID_DIMS);
    uint64_t idx_bt2 = index_of(pidx_bt2, GRID_DIMS);
    return std::make_pair(idx_bt1, idx_bt2);
}

void prepare_final_tree(message &reply, std::string worker){
    reply.type = MSG_UPDATE_BOUNDARY_TREE;
    if(!G_got_full_btree[worker]){
        if(G_reply_btt == nullptr){
            G_bound_trees.get_task(G_reply_btt);
            G_reply_bt = G_reply_btt->bt;
            // std::cout << "Final tree got from bag. Index: "<< G_reply_bt->index_to_string() << "\n\n" ;
        }
        reply.content = hps::to_string<boundary_tree>(*G_reply_bt);
        // G_reply_bt->print_tree();
        G_got_full_btree[worker]=true;
    }else{
        reply.content="";
    }
}

merge_btrees_task *align_worker_with_task(TWorkerIdx id){ // this function is called only when we have more workers than tasks (at least the same amount of workers and tasks)
    // std::cout << "align task and worker\n";
    merge_btrees_task *ret;
    size_t position = G_pool_of_workers.search_worker_by_function<TWorkerIdx>(id, get_worker_idx);
    bool got = G_merge_bag.get_task_by_position(ret, position);
    // std::cout << "position in merge bag: " << position<< "\n";
    if(got){
        return ret;
    }
    return nullptr;
    
}

merge_btrees_task *choose_task(TWorkerIdx id){
    // std::cout << "worker " << id << " chosing task\n";
    size_t position = G_pool_of_workers.search_worker_by_function<TWorkerIdx>(id, get_worker_idx);
    double prop_pos = (double) position / G_pool_of_workers.size();
    // std::cout << "position in merge bag: " << position<< "\n";
    size_t task_pos = std::round(G_merge_bag.size() * prop_pos);
    merge_btrees_task *mbt;
    auto got = G_merge_bag.get_task_by_position(mbt, task_pos);
    if(got){
        return mbt;
    }
    return nullptr;
}

merge_btrees_task *get_task_balanced(TWorkerIdx idx){
    merge_btrees_task *ret_task;
    if(G_merge_bag.size() > G_pool_of_workers.size()){ // there are more tasks than worker, so, task must be "chosen" by worker
        ret_task = choose_task(idx);
    }else{ // there at least the same amount of  workers and task, so we need to discover the worker for the greater task doing a greedy search
        ret_task = align_worker_with_task(idx);
    }
    return ret_task;
}

merge_btrees_task *get_task_naive(TWorkerIdx idx){
    merge_btrees_task *ret_task;
    if(G_merge_bag.get_task(ret_task)){
        return ret_task;
    }else{
        return nullptr;
    }
    
}




#define GET_MERGE_TASK(idx) get_task_balanced(idx)

void process_task_request(message &recv_msg, zmq::socket_t &sock){
    TWorkerIdx worker_idx = recv_msg.sender;
    message reply;
    reply.sender = 0;
    // std::cout << G_merge_bag.size() << " merge task waiting\n";
    if(!G_input_tiles.empty()){ // there is tiles of image to read and calculate maxtree yet
        prepare_tile(reply);
    // }else if(!G_merge_bag.empty()){ // there is merge to be done
    }else if(!G_merge_bag.empty()){
        // std::cout << G_merge_bag.size() << " merge task waiting\n";
        reply.type = MSG_MERGE_BOUNDARY_TREE;
        merge_btrees_task *mbt;
        mbt = GET_MERGE_TASK(worker_idx); // GET_MERGE_TASK defined as macro to easier and more efficient replace of function
        if(mbt!=nullptr){
            reply.content = hps::to_string(*mbt);
            // std::cout << "mbt:" << mbt->bt1->index_to_string() << " " << mbt->bt2->index_to_string() << "\n";
        }else{
            reply.type = MSG_NULL;
            reply.content = "";
        }
        // std::cout << "reply merge with: '" << reply.content << "'\n";
    }else if(!G_merge_bag.is_running()){ // final boundary tree is ready
        prepare_final_tree(reply,recv_msg.content);
        G_updates_sent++;
        // std::cout << "updates sent " << G_updates_sent << "\n"; 
    }else if(G_updates_sent > G_total_tiles){
        reply.type = MSG_COMMAND;
        reply.content = "END";
        G_finished_workers++;
        // std::cout << "finish work message sent";
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
        
        // std::cout << " type:"<< recv_msg.type << " -> " << NamesMessageType[recv_msg.type] << "\n";
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
    std::thread pair_maker(search_pair_naive);
    // std::thread pair_maker(search_pair);

    fill.join();
    pair_maker.join();
    receiver.join();

    finish_workers(sock);
    
}