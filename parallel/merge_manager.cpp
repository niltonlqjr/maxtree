#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <cmath>
#include <condition_variable>
#include <zmq.hpp>
#include <atomic>

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
                // try{
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
                // }
                // catch(std::runtime_error &e){
                //     std::cerr << e.what();
                //     G_bound_trees.insert_task(btt);
                // }catch(std::out_of_range &r){
                //     std::cerr << "try to access an out of range element\n";
                // }
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



void prepare_tile(message &reply){
    input_tile_task *task;
    std::pair<uint32_t,uint32_t> idx_reply;
    if(G_input_tiles.get_task(task)){
        idx_reply = std::make_pair(task->i, task->j);
        delete task;
    }else{
        idx_reply = GRID_DIMS;
    }
    /* 
    std::cout << "tile: " << int_pair_to_string(idx_reply) << " sent to " << reply.content
              << " remaining " << G_input_tiles.size() << "\n";
     */
    reply.type = MSG_TILE_IDX;
    reply.content = hps::to_string<std::pair<uint32_t,uint32_t>>(idx_reply);
}

void message_sender(zmq::socket_t &sock){
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
        string_idx = "";
        
        if(!G_registry_queue.empty()){
            G_registry_queue.get_task(registry_queue_element);
            string_idx = registry_queue_element.first;
            reply.content = std::to_string(registry_queue_element.second->get_index());
            reply.size = reply.content.size();
            reply.type = MSG_NEW_IDX;
            reply_s = hps::to_string(reply);
            G_busy_workers.insert_worker(registry_queue_element.second->get_index(), registry_queue_element.second);
            _m = "Registring: " + string_idx + " as "+ reply.content +"\n";
            std::cout << _m;
        }else if((G_input_tiles.is_running() || !G_input_tiles.empty())
                 && G_waiting_workers.size() > 0){
            prepare_tile(reply);
            w = G_waiting_workers.get_worker();
            worker_idx = w->get_index();
            G_busy_workers.insert_worker(worker_idx, w);
            string_idx = std::to_string(worker_idx);
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
            prepare_final_tree(reply, w->get_name());
            string_idx = std::to_string(w->get_index());
            reply_s = hps::to_string(reply);
            G_updates_sent.fetch_add(1);
        }
        // else if(G_updates_sent.load() >= G_total_workers.load()){
        //     w = G_waiting_workers.get_worker();
        //     string_idx = std::to_string(w->get_index());
        //     reply_s = create_end_command_msg();
        //     G_updates_sent.fetch_add(1);
        // }
        if(string_idx != ""){
            zmq::message_t msg_id(string_idx);
            zmq::message_t msg_reply(reply_s);
            G_sock_lock.lock();
            auto __ret0 = sock.send(msg_id, zmq::send_flags::sndmore);
            auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
            G_sock_lock.unlock();
            // _m = "message " +  NamesMessageType[reply.type] + " sent to index: " + string_idx + "\n";
            // std::cout << _m;
        }
    }
}


void registry_worker(message &recv_msg, std::string worker_zmq_id){
    worker w_rec = hps::from_string<worker>(recv_msg.content);
    TWorkerIdx current_idx=G_idx_at_manager++;
    worker *w_at_manager = new worker(w_rec);
    std::string _m;

    w_at_manager->update_index(current_idx);
    // G_waiting_workers.insert_worker(w_at_manager);
    
    // _m = "worker " + std::to_string(w_at_manager->get_index()) + " going to waiting queue\n";
    // std::cout << _m;
    if(G_merge_bag.is_running()){
        G_got_full_btree[w_at_manager->get_name()] = false;
    }

    auto registry_task = std::make_pair(worker_zmq_id, w_at_manager);
    G_registry_queue.insert_task(registry_task);

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
    worker *w;
    std::string _m;
    
    // _m = "worker " + std::to_string(recv_msg.sender) + " going to waiting queue\n";
    // std::cout << _m;

    
    G_bound_trees.insert_task(new_bttask);
    // w=G_busy_workers.get_worker(recv_msg.sender);
    // G_waiting_workers.insert_worker(w);
    
    // zmq::message_t msg_id(std::to_string(recv_msg.sender));
    // zmq::message_t msg_reply("ACK");
    // G_sock_lock.lock();
    // auto __ret0 = sock.send(msg_id, zmq::send_flags::sndmore);
    // auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
    // G_sock_lock.unlock();

    

}

void process_task_request(message &recv_msg){
    worker *w;
    std::string _m;
    // _m = "worker " + std::to_string(recv_msg.sender) + " going to waiting queue\n";
    // std::cout << _m;
    w = G_busy_workers.get_worker(recv_msg.sender);
    G_waiting_workers.insert_worker(w);
    /* if(G_num_merges.load() >= G_total_merges.load() && !G_got_full_btree[w->get_name()]){

    }*/
}


void manager_recv(zmq::socket_t &sock){
    zmq::message_t request,idx;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::string _m;
    uint64_t _lc=0;
    G_num_merges = 0;
    uint64_t calculated_tiles = 0;
    std::string rec_msg;
    // TWorkerIdx current_idx;
    do{
        // std::cout << "before recv in inside manager_recv\n";
        auto idx_recv = sock.recv(idx, zmq::recv_flags::none);
        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        
        rec_msg = request.to_string();
        message recv_msg = hps::from_string<message>(rec_msg);
        
        // _m = "worker: " + idx.to_string() + " requested " + NamesMessageType[recv_msg.type] + "\n";
        // std::cout << _m;
        
        if(recv_msg.type == MSG_REGISTRY){
            registry_worker(recv_msg, idx.to_string());
        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            recv_boundary_tree(recv_msg);
        }else if(recv_msg.type == MSG_SEND_MERGED_TREE){
            recv_boundary_tree(recv_msg);
            G_num_merges.fetch_add(1);
            if(G_num_merges.load() >= G_total_merges.load()){
                G_merge_bag.notify_end();
                G_bound_trees.notify_end();
            }
        }else if(recv_msg.type == MSG_GET_TASK){
            process_task_request(recv_msg);
        }
    }while(G_updates_sent.load() < G_total_workers.load());
    std::cout << "manager_recv reached last line (" << __LINE__ << ")\n";
    
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
}


int main(int argc, char *argv[]){
    
    std::string port, protocol;

    read_config(argv[1], port, protocol);
    GRID_DIMS = std::make_pair(G_glines,G_gcolumns);
    //  Prepare contexts and sockets
    
    
    G_total_merges = G_glines * (G_gcolumns - 1) + G_glines - 1;
    G_total_tiles = G_glines * G_gcolumns;
    G_updates_sent.store(0);
    G_total_workers.store(0) ;
    G_finished_workers.store(0);
    G_workers_finished.store(0);
    
    zmq::context_t context_reg(nth);
    // zmq::socket_t  sock(context_reg, zmq::socket_type::rep);
    zmq::socket_t sock(context_reg, zmq::socket_type::router);
    
    self_address = protocol+"://*:"+port;
    std::string address = self_address;
    sock.bind(address);
    
    std::cout << "running at port " << port << "\n";

    
    G_sender_running=false;
    G_input_tiles.start();
    G_bound_trees.start();
    G_merge_bag.start();
 
    std::thread fill(fill_input_bag);
    std::thread receiver(manager_recv, std::ref(sock));
    std::thread pair_maker(search_pair_naive);
    std::thread merge_task_sender(message_sender, std::ref(sock));
    // std::thread pair_maker(search_pair);

    fill.join();
    std::cout << "fill\n";
    receiver.join();
    std::cout << "receiver\n";
    merge_task_sender.join();
    std::cout << "task sender\n";
    pair_maker.join();
    std::cout << "pair maker\n";

    // finish_workers(sock);
    
}








/*

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




void _process_task_request(message &recv_msg, zmq::socket_t &sock){
    TWorkerIdx worker_idx = recv_msg.sender;
    message reply;
    reply.sender = 0;
    reply.content = recv_msg.content;
    // std::cout << "worker " << worker_idx << " request a task\n";
    worker *w;
    bool must_reply = true;
    
    // std::cout << G_merge_bag.size() << " merge task waiting\n";
    if(!G_input_tiles.empty() || G_input_tiles.is_running()){ // there is tiles of image to read and calculate maxtree yet
        prepare_tile(reply);
    // }else if(!G_merge_bag.empty()){ // there is merge to be done
    }else if(G_merge_bag.is_running()){
        // G_busy_workers.search_worker_by_function<TWorkerIdx>(worker_idx, ) 
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        try{
            w = G_busy_workers.get_worker(recv_msg.sender);
            G_waiting_workers.insert_worker(w);
            if(verbose) std::cout << "process task request line: "<< __LINE__ << "\n";
        }catch(...){
            std::string s = "--------> got worker: " + std::to_string(worker_idx) + " fails <------- ";
            s += std::to_string(G_busy_workers.size()) + " " + std::to_string(G_waiting_workers.size()) + "\n";
            std::cout << s;
        }
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        must_reply = false;
    }else if(!G_merge_bag.is_running()){ // final boundary tree is ready
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        prepare_final_tree(reply,recv_msg.content);
        G_updates_sent++;
        // std::cout << "updates sent " << G_updates_sent << "\n"; 
    }else if(G_updates_sent >= G_total_tiles){
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        reply.type = MSG_COMMAND;
        reply.content = "END";
        G_finished_workers++;
        // std::cout << "finish work message sent";
        // reply.size = 0;
    }else{
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        must_reply = false;
    }
    if(must_reply){
        if(verbose) std::cout << "process task request line: "<< __LINE__ <<"\n";
        reply.size = reply.content.size();
        std::string reply_s = hps::to_string(reply);
        std::string str_id = std::to_string(worker_idx);
        zmq::message_t msg_id(str_id);
        zmq::message_t msg_reply(reply_s);
        // G_sock_lock.lock();
        auto __ret0 = sock.send(msg_id, zmq::send_flags::sndmore);
        auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
        // G_sock_lock.unlock();
    }else{
        std::cout << " ========> not reply <==========\n";
    }
}

void _manager_recv(zmq::socket_t &sock){

    // todos os trabalhadores devem procurar um balanceamento na equação 1.
    //      trabalho_realizado = poder_de_processamento * 1/peso_das_tarefas_realizadas (1)
    // sendo assim, todas solicitações vão procurar responder com o trabalhador que tem
    // o menor coeficiente de trabalho_realizado dentre os disponíveis.
    // é uma estratégia gulosa.

    zmq::message_t request,idx;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    std::string _m;
    uint64_t _lc=0;
    G_num_merges = 0;
    uint64_t calculated_tiles = 0;
    // TWorkerIdx current_idx;
    while(G_updates_sent < G_total_tiles){
        // std::cout << "manager_recv\n";
    // while(num_merges < G_total_merges || !busy_workers.empty()){
        // std::cout << "updates: " << G_updates_sent << "\n";
        if(verbose){
            _m = "!!!!!!!! busy: " + std::to_string(G_busy_workers.size()) + " !!!!!!!!!\n";
            std::cout << _m;
        }
        // G_sock_lock.lock();
        // std::cout << "manager recv lock "<< ++_lc << "\n";
        auto idx_recv = sock.recv(idx, zmq::recv_flags::none);
        auto res_recv = sock.recv(request,zmq::recv_flags::none);
        // std::cout << "manager recv unlock "<< _lc <<"\n";
        // G_sock_lock.unlock();
        // std::cout << "message received from: " << idx.to_string() << "\n";
        std::string rec_msg;
        rec_msg = request.to_string();
        // std::cout << "rec_msg:\n"<< rec_msg << "\n========\n";
        message recv_msg = hps::from_string<message>(rec_msg);
        
        // std::cout << " type:"<< recv_msg.type << " -> " << NamesMessageType[recv_msg.type] << "\n";
        _m = "type:"+ NamesMessageType[recv_msg.type] + " sender (at message):" + std::to_string(recv_msg.sender) 
            + " sender (sock.recv):" + idx.to_string() +  "\n";
        std::cout << _m;
        if(recv_msg.type == MSG_REGISTRY){
            registry_worker(recv_msg, idx.to_string());
        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            // std::cout << "recv line: "<< __LINE__ <<"\n";
            recv_boundary_tree(recv_msg, sock);
        }else if(recv_msg.type == MSG_SEND_MERGED_TREE){
            // std::cout << "recv line: "<< __LINE__ <<"\n";
            recv_boundary_tree(recv_msg, sock);
            G_num_merges++;
            if(verbose){
                _m = " ===========> merge " + std::to_string( G_num_merges )+ " of " + std::to_string(G_total_merges) +" ends\n";
                std::cout << _m;
            }
            // busy_workers.erase(recv_msg.sender);
            if(G_num_merges >= G_total_merges){
                G_merge_bag.notify_end();
                std::cout << "merge bag end\n";
            }
        }else if(recv_msg.type == MSG_GET_TASK){
            // std::cout << "recv line: "<< __LINE__ <<"\n";
            _process_task_request(recv_msg, sock);
        }
        // std::cout << "end iteration<--------\n";
    }
    G_bound_trees.notify_end();
    // std::cout << "\n==============\nend manager_recv\n";
    
    
}





void _task_sender(zmq::socket_t &sock){
    TWorkerIdx worker_idx;
    worker *w;
    size_t pos_w;
    message reply;
    merge_btrees_task *mbt;
    std::string _m;
    while(G_merge_bag.is_running()){
        
        if(G_merge_bag.get_task(mbt)){
            if(verbose){
                _m = "?????? waiting: " + std::to_string(G_waiting_workers.size()) + " ?????????\n";
                std::cout << _m;
            }
            w = G_waiting_workers.get_worker();
            worker_idx = w->get_index();
            G_busy_workers.insert_worker(worker_idx, w);
            // std::cout << G_merge_bag.size() << " merge task waiting\n";
            reply.type = MSG_MERGE_BOUNDARY_TREE;
            // mbt = nullptr;
            // while(mbt == nullptr){
            //     mbt = GET_MERGE_TASK(w); // GET_MERGE_TASK defined as macro to easier and more efficient replace of function
                
            // }
            reply.content = hps::to_string(*mbt);
            // std::cout << "mbt:" << mbt->bt1->index_to_string() << " " << mbt->bt2->index_to_string() << "\n";
            // std::cout << "reply merge with: '" << reply.content << "'\n";
            reply.size = reply.content.size();
            std::string reply_s = hps::to_string(reply);
            zmq::message_t msg_id(std::to_string(worker_idx));
            zmq::message_t msg_reply(reply_s);
            
            // _m = "sending task to worker: " + std::to_string(worker_idx) + "\n";
            // std::cout << _m;
            // G_sock_lock.lock();
            // std::cout << "task sender lock\n";
            auto __ret0 = sock.send(msg_id, zmq::send_flags::sndmore);
            auto reply_return = sock.send(msg_reply, zmq::send_flags::none);
            // std::cout << "task sender unlock\n";
            // G_sock_lock.unlock();
            // _m = "task has been sent to worker: " + std::to_string(worker_idx) + "\n";
            // std::cout << _m;
        }
    }
    
}

*/