#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <cmath>
#include <condition_variable>
#include <zmq.hpp>
#include <atomic>
#include <unordered_map>

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


uint32_t G_glines, G_gcolumns;
std::atomic<uint64_t> G_updates_sent, G_finished_workers, G_num_merges, G_workers_finished;
std::atomic<uint64_t> G_total_merges,  G_total_tiles, G_total_workers;

bool print_only_trees;
bool verbose;

bool G_sender_running;


static const int nth = 1;

TWorkerIdx G_idx_at_manager=1; //index count of workers. Index 0 is for manager

boundary_tree *G_reply_bt=nullptr;
boundary_tree_task *G_reply_btt=nullptr;


hash_scheduler_of_worker<TWorkerIdx, worker*> G_busy_workers;
scheduler_of_workers<worker*> G_waiting_workers;
// ordered_scheduler_of_workers <worker*, worker_lesser_than> G_waiting_workers;


bag_of_tasks<input_tile_task* > G_input_tiles;
bag_of_tasks<boundary_tree_task *> G_bound_trees;
bag_of_tasks<merge_btrees_task *> G_merge_bag;
// prio_bag_of_tasks<merge_btrees_task *> G_merge_bag;
// ordered_bag_of_tasks<merge_btrees_task *, mbt_lesser_than> G_merge_bag;

bag_of_tasks< std::pair<std::string, worker *> > G_registry_queue(true);



// given that unordered_map needs a hash function for pair and there are few grids, a map is enought
std::map <std::pair<uint32_t,uint32_t>, worker *> G_grid_id_worker;

std::unordered_map<std::string, bool> G_got_full_btree;

std::mutex G_sock_lock, G_scheduler_lock, G_sender_lock;

std::condition_variable_any G_sender_cv;


void read_config(char conf_name[], std::string &port_recv, std::string &port_send,  std::string &protocol){

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

    port_recv = get_field(configs, "port_recv", DEFAULT_RECV_PORT);
    port_send = get_field(configs, "port_send", DEFAULT_SEND_PORT);
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




std::pair<uint32_t, uint32_t> next_tile(std::pair<uint32_t, uint32_t> p){
    std::pair<uint32_t, uint32_t> newp;
    newp.second = (p.second + 1) % GRID_DIMS.second;
    newp.first = p.first + (newp.second != p.second+1);
    return newp;
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
            std::cout << "get final tree...\n";
            G_bound_trees.get_task(G_reply_btt);
            G_reply_bt = G_reply_btt->bt;
            std::cout << "Final tree got from bag. Index: "<< G_reply_bt->index_to_string() << "\n\n" ;
        }
        std::cout << "tree "<< G_reply_bt->index_to_string() << " was put at reply to worker: " << worker << "\n\n" ;
        reply.content = hps::to_string<boundary_tree>(*G_reply_bt);
        // G_reply_bt->print_tree();
        G_got_full_btree[worker]=true;
    }else{
        reply.content="";
    }
}

merge_btrees_task *align_worker_with_task(worker *w){ // this function is called only when we have more workers than tasks (at least the same amount of workers and tasks)
    // std::cout << "align task and worker\n";
    /* merge_btrees_task *ret;
    
    bool got = G_merge_bag.get_task_by_position(ret, position);
    // std::cout << "position in merge bag: " << position<< "\n";
    if(got){
        return ret;
    } */
    return nullptr;
    
}

merge_btrees_task *choose_task(worker *w){
    // std::cout << "worker " << id << " chosing task\n";
    /* 
    double prop_pos = (double) position / G_busy_workers.size();
    // std::cout << "position in merge bag: " << position<< "\n";
    size_t task_pos = std::round(G_merge_bag.size() * prop_pos);
    merge_btrees_task *mbt;
    bool got = G_merge_bag.get_task_by_position(mbt, task_pos);
    if(got){
        return mbt;
    } */
    return nullptr;
}

merge_btrees_task *get_task_balanced(worker *w){
    merge_btrees_task *ret_task;
    if(G_merge_bag.size() > G_busy_workers.size()){ // there are more tasks than worker, so, task must be "chosen" by worker
        ret_task = choose_task(w);
    }else{ // there at least the same amount of  workers and task, so we need to discover the worker for the greater task doing a greedy search
        ret_task = align_worker_with_task(w);
    }
    return ret_task;
}

merge_btrees_task *get_task_naive(worker *w){
    merge_btrees_task *ret_task;
    if(G_merge_bag.get_task(ret_task)){
        return ret_task;
    }else{
        return nullptr;
    }
    
}


#define GET_MERGE_TASK(wrk) get_task_naive(wrk)

inline std::string create_end_command_msg(){
    message cmd;
    cmd.type = MSG_COMMAND;
    cmd.content = "END";
    cmd.size = cmd.content.size();
    return hps::to_string(cmd);
}



void prepare_tile(message &reply, std::string widx){
    input_tile_task *task;
    std::pair<uint32_t,uint32_t> idx_reply;
    if(G_input_tiles.get_task(task)){
        idx_reply = std::make_pair(task->i, task->j);
        delete task;
    }else{
        idx_reply = GRID_DIMS;
    }
     
    std::string _m;
    _m = "tile: " + int_pair_to_string(idx_reply) + " sent to " + widx
       + " remaining " + std::to_string(G_input_tiles.size()) + "\n";
    
    std::cout << _m;
    
    reply.type = MSG_TILE_IDX;
    reply.content = hps::to_string<std::pair<uint32_t,uint32_t>>(idx_reply);
}



void registry_worker(message &recv_msg, std::string worker_zmq_id, zmq::socket_t &sock){
    worker w_rec = hps::from_string<worker>(recv_msg.content);
    TWorkerIdx current_idx=G_idx_at_manager++;
    worker *w_at_manager = new worker(w_rec);
    std::string _m, string_idx, reply_s;
    message reply;

    w_at_manager->update_index(current_idx);

    // _m = "worker " + std::to_string(w_at_manager->get_index()) + " going to waiting queue\n";
    // std::cout << _m;
    if(G_merge_bag.is_running()){
        G_got_full_btree[w_at_manager->get_name()] = false;
    }

   
    string_idx = worker_zmq_id;
    reply.content = std::to_string(w_at_manager->get_index());
    reply.size = reply.content.size();
    reply.type = MSG_NEW_IDX;
    reply_s = hps::to_string(reply);
    G_busy_workers.insert_worker(w_at_manager->get_index(), w_at_manager);
    
    _m = "Registring: " + string_idx + " as "+ reply.content +"\n";
    std::cout << _m;

    zmq::message_t msg_id(string_idx);
    zmq::message_t msg_reply(reply_s);
    // G_sock_lock.lock();
    auto __ret0 = sock.send(msg_id, zmq::send_flags::sndmore);
    auto __ret1 = sock.send(msg_reply, zmq::send_flags::none);
    

    {//unique_lock scope
        std::unique_lock<std::mutex>(G_sender_lock);
        G_total_workers.fetch_add(1);
        G_sender_cv.notify_one();
    }
    // _m = "response enqued: " + worker_zmq_id + "," + std::to_string(current_idx) + " source line: " + std::to_string( __LINE__ ) +"\n";
    // std::cout << _m;
}

void recv_boundary_tree(message &recv_msg){
    boundary_tree_task btt = hps::from_string<boundary_tree_task>(recv_msg.content);
    boundary_tree_task *new_bttask = new boundary_tree_task(btt);
    std::string _m;
        
    G_bound_trees.insert_task(new_bttask);

    // _m = "received: " + new_bttask->bt->index_to_string() + " distance:" + int_pair_to_string(new_bttask->nb_distance) + "\n";
    // std::cout << _m;

    // w=G_busy_workers.get_worker(recv_msg.sender);
    // G_waiting_workers.insert_worker(w);
}

void process_task_request(message &recv_msg){
    worker *w;
    std::string _m;
    // _m = "worker " + std::to_string(recv_msg.sender) + " going to waiting queue\n";
    // std::cout << _m;
    w = G_busy_workers.get_worker(recv_msg.sender);
    G_waiting_workers.insert_worker(w);
}


/* void search_pair_naive(){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s, _m;
    uint64_t self_int_idx, nb_int_idx;
    while(G_merge_bag.is_running()){ // || G_bound_trees.size() > 1){
        
        bool got = G_bound_trees.get_task(btt);
        _m = "G_merge_bag size:" + std::to_string(G_merge_bag.size()) + " G_bound_trees size:" + std::to_string(G_bound_trees.size()) +"\n" ;
        _m += "Got btree:" + btt->bt->index_to_string() + " distance:" + int_pair_to_string(btt->nb_distance) + "\n";
        std::cout << _m;
        
        if(got){
            self_int_idx = index_of(btt->index, GRID_DIMS);
            merge_dir = btt->define_merge_direction();
            nb_direction = btt->define_nb_direction(merge_dir);
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){

                auto got_n = G_bound_trees.get_task_by_function<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                if(!got_n){
                    _m = "tree "+ btt->bt->index_to_string() + " has no neighbor on bag\n";
                    std::cout << _m;
                    G_bound_trees.insert_task(btt);

                }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){
                    auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                    G_merge_bag.insert_task(new_merge_task);
                }else{
                    G_bound_trees.insert_task(n);
                    G_bound_trees.insert_task(btt);
                    _m = "tree btt: "+ btt->bt->index_to_string() + " and n:" + n->bt->index_to_string() + " are waiting for different distances\n"
                        + "btt:" + int_pair_to_string(btt->nb_distance) + " n:" + int_pair_to_string(n->nb_distance) + "\n";
                    std::cout << _m;
                }

            }else if (inside_rectangle(btt->index, GRID_DIMS)){ // the neighbor of btt is not inside the grid (it does not exist, so go to next merge)
                if(btt->nb_distance.first == 0){
                    if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
                        // if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " line distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                        // }
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
            else{
                std::cout << "------------------->nao deveria entrar aqui!\n";
            }
        }
    }
    // if(verbose) std::cout << "end worker search pair\n";
    // std::cout << "end worker search pair\n";
}
 */
void search_pair(){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir,got_n;
    std::string s, _m;
    uint64_t self_int_idx, nb_int_idx, nb_linear_idx, btt_linear_idx;
    std::unordered_map<uint64_t, boundary_tree_task*> waiting_pair;
    
    while(G_merge_bag.is_running()){
        bool got = G_bound_trees.get_task(btt); /* <========= thread 6 */
        if(!G_merge_bag.is_running()){
            if(got){
                G_bound_trees.insert_task(btt);
            }
            break;
        }
        // _m = "task tile: " + btt->bt->index_to_string() + "\n";
        self_int_idx = index_of(btt->index, GRID_DIMS);
        merge_dir = btt->define_merge_direction();
        nb_direction = btt->define_nb_direction(merge_dir);
        idx_nb = btt->neighbor_idx(nb_direction);
        if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
            got_n = false;
            nb_linear_idx = index_of(idx_nb, GRID_DIMS);
            if(waiting_pair.find(nb_linear_idx) != waiting_pair.end()){
                got_n = true;
                n = waiting_pair[nb_linear_idx];
                _m = "---------------- remove from waiting " + n->bt->index_to_string() + " distance: " + int_pair_to_string(n->nb_distance) 
                   + "linear: " + std::to_string(nb_linear_idx)+ " ------------------\n";
                // std::cout << _m;
            }else{
                got_n = G_bound_trees.get_task_by_function<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
            }
            if(got_n){
                // if(n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){
                if(btt->can_merge_with(n)){
                    auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                    G_merge_bag.insert_task(new_merge_task);


                    _m = "new merge " + btt->bt->index_to_string() + n->bt->index_to_string()
                       + "    \tdistance:" + int_pair_to_string(btt->nb_distance)+"\n";
                    // std::cout << _m;
                }else{
                    if(btt->nb_distance.first < n->nb_distance.first){
                        aux = btt;
                        btt = n;
                        n = aux;
                        std::cout << "swap\n";
                    }else if(btt->nb_distance.first == n->nb_distance.first && btt->nb_distance.second < n->nb_distance.second){
                        aux = btt;
                        btt = n;
                        n = aux;
                        std::cout << "swap\n";
                    }
                    // waiting_pair[nb_linear_idx] = n;
                    G_bound_trees.insert_task(n);
                    btt_linear_idx = index_of(btt->index, GRID_DIMS);
                    waiting_pair[btt_linear_idx] = btt;
                    _m = "++++++++++++ going to waiting " + btt->bt->index_to_string() + " distance: " + int_pair_to_string(btt->nb_distance) 
                    + "linear: " + std::to_string(btt_linear_idx) + " +++++++++++++++++\n";
                    // std::cout << _m;
                }
            }else{
                btt_linear_idx = index_of(btt->index, GRID_DIMS);
                waiting_pair[btt_linear_idx] = btt;
                // G_bound_trees.insert_task(btt);
                _m = "============ going to waiting " + btt->bt->index_to_string() + " distance: " + int_pair_to_string(btt->nb_distance) 
                    + "linear: " + std::to_string(btt_linear_idx) + " ============\n";
                // std::cout << _m;
            }
        }else if (inside_rectangle(btt->index, GRID_DIMS)){
            if(btt->nb_distance.first == 0){
                if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
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
        }else{
            std::cout << "====> nenhuma das tasks dentro do grid!\n";
        }
    }
    std::cout << "search_pair end<=======\n";
}



void manager_recv(zmq::socket_t &sock_recv){
    zmq::message_t request,idx;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::string _m;
    uint64_t _lc=0;
    G_num_merges.store(0);
    uint64_t calculated_tiles = 0;
    std::string rec_msg;
    // TWorkerIdx current_idx;
    do{
        // std::cout << "before recv in inside manager_recv\n";
        // G_sock_lock.lock();
        // std::cout << "manager recv waiting message\n";
        auto idx_recv = sock_recv.recv(idx, zmq::recv_flags::none);/* <======== thread 5*/
        auto res_recv = sock_recv.recv(request,zmq::recv_flags::none);
        // std::cout << "message received\n";
        // G_sock_lock.unlock();
        
        rec_msg = request.to_string();
        message recv_msg = hps::from_string<message>(rec_msg);
        
        // _m = "worker: " + idx.to_string() + " requested " + NamesMessageType[recv_msg.type];
        // if(recv_msg.type == MSG_COMMAND){
        //     _m += " " + recv_msg.content;
        // }
        // _m += "\n";
        std::cout << _m;
        
        if(recv_msg.type == MSG_REGISTRY){
            registry_worker(recv_msg, idx.to_string(), sock_recv);
        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            recv_boundary_tree(recv_msg);
        }else if(recv_msg.type == MSG_SEND_MERGED_TREE){
            recv_boundary_tree(recv_msg);
            G_num_merges.fetch_add(1);
            if(G_num_merges.load() >= G_total_merges.load()){
                G_merge_bag.notify_end();
                G_bound_trees.notify_end();
                std::cout << "end merges\n";
            }
        }else if(recv_msg.type == MSG_GET_TASK){
            process_task_request(recv_msg);
        }else if(recv_msg.type == MSG_COMMAND){
            if(recv_msg.content == "FINISH"){
                G_workers_finished.fetch_add(1);
            }
        }
    }while(G_updates_sent.load() < G_total_workers.load() 
        || G_workers_finished.load() < G_total_workers.load());
    std::cout << "manager_recv reached last line (" << __LINE__ << ")<==========\n";
    
}

void message_sender(zmq::socket_t &sock_send){
    TWorkerIdx worker_idx;
    worker *w;
    size_t pos_w;
    message reply;
    merge_btrees_task *mbt;
    std::string _m, string_idx, reply_s;
    std::pair< std::string, worker *> registry_queue_element;

    {//unique_lock scope
        std::unique_lock<std::mutex> l(G_sender_lock);
        if(G_total_workers.load() <= 0){
            std::cout << "sender waiting for workers\n";
            G_sender_cv.wait(l);
        }
    }

    while(G_updates_sent.load() < G_total_workers.load()){
        string_idx = "NO_WORKER";
        if((G_input_tiles.is_running() || !G_input_tiles.empty())){
            w = G_waiting_workers.get_worker();
            worker_idx = w->get_index();
            G_busy_workers.insert_worker(worker_idx, w);
            string_idx = std::to_string(worker_idx);
            prepare_tile(reply, string_idx);
            reply_s = hps::to_string(reply);
        }else if(G_merge_bag.is_running() && !G_merge_bag.empty()
                 && G_waiting_workers.size() > 0){
            w = G_waiting_workers.get_worker();
            worker_idx = w->get_index();
            G_busy_workers.insert_worker(worker_idx, w);
            reply.type = MSG_MERGE_BOUNDARY_TREE;
            G_merge_bag.get_task(mbt);
            reply.content = hps::to_string(*mbt);
            reply.size = reply.content.size();
            string_idx = std::to_string(worker_idx);
            reply_s = hps::to_string(reply);
        }else if(!G_merge_bag.is_running()){
            w = G_waiting_workers.get_worker();
            worker_idx = w->get_index();
            G_busy_workers.insert_worker(worker_idx, w);
            prepare_final_tree(reply, w->get_name());
            string_idx = std::to_string(w->get_index());
            reply_s = hps::to_string(reply);
            G_updates_sent.fetch_add(1);
        }

        if(string_idx != "NO_WORKER"){
            zmq::message_t msg_id(string_idx);
            zmq::message_t msg_reply(reply_s);
            auto __ret0 = sock_send.send(msg_id, zmq::send_flags::sndmore);
            auto reply_return = sock_send.send(msg_reply, zmq::send_flags::none);
            // _m = "message " +  NamesMessageType[reply.type] + " sent to index: " + string_idx + "\n";
            // std::cout << _m;
        }
    }
    std::cout << "message_sender end<===========\n";
}



void fill_input_bag(){
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::pair<uint32_t,uint32_t> grid_idx = current_tile;

    while(inside_rectangle(grid_idx,GRID_DIMS)){    
        G_input_tiles.insert_task(new input_tile_task(grid_idx));
        grid_idx = next_tile(current_tile);               
        current_tile = grid_idx;
    }
    G_input_tiles.notify_end();
}

/* 
void finish_workers(zmq::socket_t &sock){
    std::string sout = std::to_string(G_finished_workers) + " of " + std::to_string(G_total_workers.load()) + " workers finisehd before function finish_workers\n";
    std::cout << sout;
    zmq::message_t request, idx;
    message reply;
    worker *w;
    while(G_waiting_workers.size() > 0){
        w = G_waiting_workers.get_worker();
        std::string reply_s = create_end_command_msg();
        zmq::message_t msg_reply(reply_s);
        zmq::message_t msg_idx(std::to_string(w->get_index()));
        auto __ret0 = sock.send(msg_idx, zmq::send_flags::sndmore);
        auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
        G_finished_workers.fetch_add(1);
        sout = "finished worker: " + std::to_string(w->get_index()) + "\ttotal worker finished: " + std::to_string(G_finished_workers) + "\n";
        std::cout << sout;
    }
    std::cout << "Waiting workers queue end\n";
    while(G_finished_workers.load() < G_total_workers.load()){
        // G_sock_lock.lock();
        auto idx_recv = sock.recv(idx, zmq::recv_flags::none);
        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        // G_sock_lock.unlock();
        std::string rec_msg;
        rec_msg = request.to_string();
        message recv_msg = hps::from_string<message>(rec_msg);
        
        reply.type = MSG_COMMAND;
        reply.content = "END";
        // reply.size = 0;
        reply.size = reply.content.size();
        std::string reply_s = hps::to_string(reply);
        zmq::message_t msg_reply(reply_s);
        zmq::message_t msg_idx(std::to_string(recv_msg.sender));
        // G_sock_lock.lock();
        auto __ret0 = sock.send(msg_idx, zmq::send_flags::sndmore);
        auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
        // G_sock_lock.unlock();
        G_finished_workers.fetch_add(1);
        sout = "finished worker: " + std::to_string(recv_msg.sender) + "\ttotal worker finished: " + std::to_string(G_finished_workers) + "\n";
        std::cout << sout;
    }
} */


int main(int argc, char *argv[]){
    
    std::string port_recv, port_send, self_address_recv, self_address_send ,protocol;

    read_config(argv[1], port_recv, port_send, protocol);
    GRID_DIMS = std::make_pair(G_glines,G_gcolumns);
    //  Prepare contexts and sockets
    
    
    G_total_merges.store(G_glines * (G_gcolumns - 1) + G_glines - 1);
    G_total_tiles.store(G_glines * G_gcolumns);
    G_updates_sent.store(0);
    G_total_workers.store(0) ;
    G_finished_workers.store(0);
    G_workers_finished.store(0);
    
    zmq::context_t context_send(nth);
    zmq::context_t context_recv(nth);
    
    // zmq::socket_t  sock(context_reg, zmq::socket_type::rep);
    zmq::socket_t sock_send(context_send, zmq::socket_type::router);
    zmq::socket_t sock_recv(context_recv, zmq::socket_type::router);
    
    self_address_recv = protocol+"://*:"+port_recv;
    sock_recv.bind(self_address_recv);
    
    self_address_send = protocol+"://*:"+port_send;
    sock_send.bind(self_address_send);

    std::cout << "receiver socket running at port " << port_recv << "\n";
    std::cout << "sender socket running at port " << port_send << "\n";


    
    G_sender_running=false;
    G_input_tiles.start();
    G_bound_trees.start();
    G_merge_bag.start();

 
    // std::thread fill(fill_input_bag);
    fill_input_bag();
    std::thread receiver(manager_recv, std::ref(sock_recv));
    // std::thread pair_maker(search_pair_naive);
    std::thread pair_maker(search_pair);
    std::thread merge_task_sender(message_sender, std::ref(sock_send));
    // std::thread pair_maker(search_pair);

    // fill.join();
    std::cout << "fill\n";
    receiver.join();
    std::cout << "receiver\n";
    merge_task_sender.join();
    std::cout << "task sender\n";
    pair_maker.join();
    std::cout << "pair maker\n";

    // finish_workers(sock);
    sock_recv.close();
    sock_send.close();
}


