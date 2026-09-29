#include "workers.hpp"


std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t){
    return t->index;
}


worker::worker(worker &w){
    this->attr = w.attr;
    this->id = w.id;
    this->name = w.name;
}

worker::worker(TWorkerIdx id, zmq::context_t &ctx ,std::string manager_send, std::string manager_recv, std::string name, std::unordered_map<std::string, TWorkerAttr> *attr){
    if(attr == nullptr){
        this->attr = new std::unordered_map<std::string, TWorkerAttr>();
    }else{
        this->attr = attr;
    }
    this->id = id;
    this->name = name;
    this->c = connection(ctx, manager_send, manager_recv, zmq::socket_type::dealer);
}

worker::worker(TWorkerIdx id, std::string manager_send, std::string manager_recv, std::string name, std::unordered_map<std::string, TWorkerAttr> *attr){
    if(attr == nullptr){
        this->attr = new std::unordered_map<std::string, double>();
    }else{
        this->attr = attr;
    }
    this->name = name;
    this->c = connection();
    this->c.set_addresses(manager_send, manager_recv);
}

worker::worker(){
    this->attr = new std::unordered_map<std::string, TWorkerAttr>();
    this->id = "";
    this->name = "";
    this->c = connection();
}

void worker::set_attr(std::string attr_name, TWorkerAttr attr_val){
    (*this->attr)[attr_name] = attr_val;
}

void worker::update_remote_attr(std::string attr_name, TWorkerAttr attr_val){
    (*this->attr)[attr_name] = attr_val;
    std::pair<std::string, TWorkerAttr> send_attr = std::make_pair(attr_name, attr_val);
    std::string content = hps::to_string(send_attr);
    message m(content, content.size(), MSG_UPDATE_WORKER, this->id);
    std::string s_msg = hps::to_string<message>(m);
    this->c.send_message(this->id, s_msg);  
    // zmq::message_t msg_0mq(s_msg);
    // this->server_sock_recv.send(msg_0mq, zmq::send_flags::none);

}

TWorkerAttr worker::get_attr(std::string s){
    if(this->attr->find(s) != this->attr->end()){
        return this->attr->at(s);
        
    }
    std::string err_msg;
    err_msg = "worker\n" + this->to_string() + "\n has no attribute '"+ s +"'";
    throw std::out_of_range(err_msg);
}

Tprocess_power worker::get_process_power(){

    return calculate_process_power(this->attr);
}

std::string worker::get_name(){
    return this->name;
}

bool worker::operator<(worker &r) {
    return this->get_process_power() < r.get_process_power();
}

bool worker::operator>(worker &r){
    return this->get_process_power() > r.get_process_power();
}

bool worker::operator==(worker &r){
    return this->get_process_power() == r.get_process_power();
}

TWorkerIdx worker::get_index(){
    return this->id;
}

void worker::update_index(TWorkerIdx new_idx){
    this->id = new_idx;
}

std::string worker::to_string(){
    std::ostringstream wstr;
    wstr << "local id:" << this->id << " -- ";
    wstr << "attributes:\n";
    for(auto elem: *this->attr){
        wstr << "(" << elem.first << "," << elem.second << ") ";
    }
    wstr << "\n";
    return wstr.str();
}

void worker::print(){
    std::cout << this->to_string();
}

void worker::get_boundary_tree(bag_of_tasks<maxtree_task *> &maxtrees,
                               bag_of_tasks<boundary_tree_task *> &boundary_trees, 
                               bag_of_tasks<maxtree_task *>  &maxtree_dest){
    bool got_task;
    maxtree_task *mtt;
    boundary_tree_task *btt;
    bag_of_tasks<maxtree_task *> maxtree_aux;
    got_task = maxtrees.get_task(mtt);
    if(got_task){
        this->busy = true;
        if(verbose){
            std::string stmt = std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) +"\n";
            stmt += mtt->mt->to_string(GLOBAL_IDX,false) + "\n";
            std::cout << stmt;
        }
        btt = new boundary_tree_task(mtt, std::make_pair<uint32_t,uint32_t>(0,1));
        boundary_trees.insert_task(btt);
        maxtree_dest.insert_task(mtt);
        this->busy = false;
    }

}

void worker::maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, bag_of_tasks<maxtree_task *> &max_trees){
    bool got_task;
    input_tile_task *t;
    maxtree_task *mt;
    
    if(verbose){
        std::ostringstream os("");
        os << "thread " << this->id << " trying to get task\n";
        std::string s = os.str();
        std::cout << s;
    }
    got_task=bag_tiles.get_task(t);
    if(got_task){
        this->busy = true;
        if(verbose){
            std::ostringstream os("");
            os << "worker " <<  this->id << " got task " << t->i << ", " << t->j << " to calculate maxtree\n";
            std::string s = os.str();
            std::cout << s;
        }
        mt = new maxtree_task(t);
        max_trees.insert_task(mt);
        this->busy = false;
    }
    if(verbose){
        std::ostringstream os("");
        os << "thread " << this->id << " couldnt get task\n";
        std::string s = os.str();
        std::cout << s; 
    }
    
}


void worker::merge_local(bag_of_tasks<merge_btrees_task *> &merge_bag, bag_of_tasks<boundary_tree_task *> &btrees_bag){
    merge_btrees_task *mbt;
    boundary_tree_task *btt;
    boundary_tree *nbt;
    std::pair<uint32_t, uint32_t> dist;
    std::string s;
    while(merge_bag.is_running() || !merge_bag.empty()){

        bool got_mt = merge_bag.get_task(mbt);
        if(got_mt){
            // if(verbose){
            #ifdef VERBOSE
                s = "-------------------TREE 1------------------\n";
                // mbt->bt1->print_tree();
                s+=mbt->bt1->border_to_string();
                s+=mbt->bt1->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                s = "-------------------TREE 2------------------\n";
                // mbt->bt2->print_tree();
                s+=mbt->bt2->border_to_string();
                s+=mbt->bt2->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                std::cout << s;
                s = "task will merge: " + std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + " \n";
                std::cout << s;
            // }
            #endif
            nbt = mbt->execute();
            
            dist.second = (mbt->bt2->grid_j - mbt->bt1->grid_j) * 2;
            dist.first = (mbt->bt2->grid_i - mbt->bt1->grid_i) * 2;
            if(dist.second >= GRID_DIMS.second){
                dist.first = 1;
                dist.second = 0;
            }else if(dist.second == 0 && dist.first >= GRID_DIMS.first){
                if(verbose){
                    std::string s = "ending  merge --- dist:" + int_pair_to_string(dist) + " " ;
                    s += std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                    s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + "\n";
                    std::cout << s;
                }
                merge_bag.notify_end();
                btrees_bag.notify_end();
            }
            #ifdef VERBOSE
            // if(verbose)
                nbt->print_tree();
            #endif
            btt = new boundary_tree_task(nbt, dist);
            #ifdef VERBOSE
            // if(verbose){
                s += " task inserted with index:" + std::to_string(btt->bt->grid_i) + ", " + std::to_string(btt->bt->grid_j);
                s += " and distance " + int_pair_to_string(btt->nb_distance) + "\n";
                std::cout << s;
            // }
            #endif
            btrees_bag.insert_task(btt);
        }
    }
    #ifdef VERBOSE
    // if(verbose) 
        std::cout << "end worker local merge\n";
    #endif
}

void worker::update_filter(bag_of_tasks<maxtree_task *> &src, bag_of_tasks<maxtree_task *> &dest, boundary_tree *global_bt, Tattribute lambda){
    bool got_task;
    maxtree_task *mtt;
    std::string s;
    got_task = src.get_task(mtt);
    #ifdef VERBOSE
    // if(verbose) 
        std::cout << "worker update\n";
        s = "updating (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") \n";
        std::cout << s;
    // }
    #endif
    if(got_task){
        this->busy = true;
        mtt->mt->update_from_boundary_tree(global_bt);
        dest.insert_task(mtt);
        mtt->mt->filter(lambda, global_bt);
        this->busy = false;
    }
    #ifdef VERBOSE
    // if(verbose) {
        s = "task of grid (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") update\n";
        std::cout << s;
    // }
    #endif
    
}


void worker::registry(){
    std::string msg_content = hps::to_string(*this);
    message requirement(msg_content, msg_content.size(), MSG_REGISTRY, this->id);
    std::string s_msg = hps::to_string(requirement);
    std::string str, _m;
    zmq::message_t message_0mq(s_msg);
    zmq::message_t reply_0mq;
    
    
    TConnectionIdx new_idx = this->c.registry();
    #ifdef VERBOSE
    if(verbose){
        _m = "send " + std::to_string(this->id) + "\n";
        std::cout << _m;
    }
    if(verbose){
        _m = "recv " + std::to_string(this->id) + " - registration successful\n";
        std::cout << _m;
    }
    #endif
    {   
        using std::to_string;
        this->update_index(to_string(new_idx));
    }
    #ifdef VERBOSE
    if(verbose){
        str+="new id: " +std::to_string(this->get_index());
        std::cout << str + "\n";
    }
    #endif

}

message worker::request_work(){
    // std::cout << "request_work\n";
    zmq::message_t reply_zmq;
    std::string reply_str;
    zmq::message_t idx;
    std::string _m, s_msg;
    if(!this->c.is_connected()){
        std::cerr << "worker " << this->id << "not connected!\n";
        std::string s("");
        return message(s,0,MSG_NULL,this->id);
    }
    
    message request(this->name, this->name.size(), MSG_GET_TASK, this->get_index());
    
    s_msg = hps::to_string<message>(request);
    
    this->c.send_message(this->get_index(), s_msg);
    
    #ifdef VERBOSE
    if(verbose) {
        _m = "request work waiting response for worker "+ std::to_string(this->get_index()) +"\n";
        std::cout << _m;
    }
    if(verbose){ 
        _m = "worker "+ std::to_string(this->get_index()) +" get response\n";
        std::cout << _m;
    }
    #endif
    return hps::from_string<message>(reply_str);
}

void worker::finish_worker(){
    message m;
    std::string content= "FINISH";
    message finish(content, content.size(), MSG_COMMAND, this->get_index());
    std::string s_finish = hps::to_string(finish);
    
    this->c.send_message(this->get_index(), s_finish);
}

void thread_check_event(connection::handshake_monitor &hsmonitor){
    hsmonitor.check_event(-1);
}

void worker::connect(){
    this->c.connect();
    // std::string _m;
    // if(!this->connected){
    //     if(this->registered){
    //         this->server_sock_send.set(zmq::sockopt::routing_id, std::to_string(this->id));
    //         this->server_sock_recv.set(zmq::sockopt::routing_id, std::to_string(this->id));
    //         if(verbose){
    //             _m = "set routing id to:" + std::to_string(this->id) + "\n";
    //             std::cout << _m;
    //         }
    //     }else{
    //         _m = "worker: " + std::to_string(this->id) + " not registered at server " + this->manager_recv + " \n";
    //         std::cerr << _m;
    //     }
    //     // connection::handshake_monitor monitor_send, monitor_recv;
        
    //     std::string monitor_send_name = "inproc://monitor_send" + std::to_string(this->id);
    //     connection::handshake_monitor monitor_send(monitor_send_name);
    //     monitor_send.init(this->server_sock_send, monitor_send_name, ZMQ_EVENT_HANDSHAKE_SUCCEEDED);
    //     this->server_sock_send.set(zmq::sockopt::linger, 0);
    //     std::thread monitor_thread_send(thread_check_event, std::ref(monitor_send));
    //     this->server_sock_send.connect(this->manager_send);
    //     monitor_thread_send.join();

    //     #ifdef VERBOSE
    //     // if(verbose){
    //         _m = std::to_string(this->get_index()) + "connected server_sock_send at: " + this->manager_send + "\n";
    //         std::cout << _m;
    //     // }
    //     #endif

    //     std::string monitor_recv_name = "inproc://monitor_recv" + std::to_string(this->id);
    //     connection::handshake_monitor monitor_recv(monitor_recv_name);
    //     monitor_recv.init(this->server_sock_recv, monitor_recv_name, ZMQ_EVENT_HANDSHAKE_SUCCEEDED);
    //     this->server_sock_recv.set(zmq::sockopt::linger, 0);
    //     std::thread monitor_thread_recv(thread_check_event, std::ref(monitor_recv));
    //     this->server_sock_recv.connect(this->manager_recv);
    //     monitor_thread_recv.join();
    //     #ifdef VERBOSE
    //     if(verbose){
    //         _m = std::to_string(this->get_index()) + "connected server_sock_recv at: " + this->manager_recv + "\n";
    //         std::cout << _m;
    //     }
    //     #endif
    // }
    // this->connected = true;
}

void worker::disconnect(){
    this->c.disconnect();
    // std::string _m;
    // if(this->connected){
    //     this->server_sock_recv.disconnect(this->manager_recv);
    //     this->server_sock_send.disconnect(this->manager_send);
    //     if(verbose){
    //         _m = std::to_string(this->id) + " disconnected from " + this->manager_send + " and " + this->manager_recv +  "\n";
    //         std::cout << _m;
    //     }
    // }
    // this->connected = false;
}

void worker::close_sockets(){
    this->c.close_sockets();
}

void worker::send_btree_task(boundary_tree_task *btt, enum message_type type){
    if(!this->c.is_connected()){
        std::string _m = "worker " + this->id + " not connected\n";
        std::cerr << _m;
        return;
    }
    std::string msg_content = hps::to_string(*btt);
    message m = message(msg_content, msg_content.size(), type, this->id);
    std::string _m;
    std::string s_msg = hps::to_string(m);
    
    if(verbose){
        _m = this->id + " -------> sending: " + NamesMessageType[type] + btt->bt->index_to_string() 
           + " distance: " + int_pair_to_string(btt->nb_distance) + "\n";
        std::cout << _m;
    }

    this->c.send_message(this->id, s_msg);

    if(verbose){
        _m = "worker " + this->get_index() + " sent message \n";
        std::cout << _m;
    }
}


void worker::send_boundary_tree(boundary_tree *bt){
    if(!this->c.is_connected()){
        std::cerr << "worker " << this->id << "not connected!\n";
        return;
    }
    std::string msg_content = hps::to_string(*bt);
    message m = message(msg_content, msg_content.size(), MSG_BOUNDARY_TREE, this->id);
    std::string s_msg = hps::to_string(m);
    this->c.send_message(this->id, s_msg);
    std::cout << "tree sent\n";   
}


bool worker_lesser_than(worker *l, worker *r){
    return l->get_process_power() < r->get_process_power();
}


void worker::search_pair(bag_of_tasks<boundary_tree_task *> &btrees_bag, 
                         bag_of_tasks<merge_btrees_task *> &merge_bag){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    // while(btrees_bag.is_running() || !btrees_bag.get_num_task() > 1){
        bool got = btrees_bag.get_task(btt);
        if(got){
            this->busy = true;
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
                    auto got_n = btrees_bag.get_task_by_function<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    
                    if(!got_n){
                        btrees_bag.insert_task(btt);
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
                        btrees_bag.insert_task(n);
                        btrees_bag.insert_task(btt);
                    }
                }
                catch(std::runtime_error &e){
                    std::cerr << e.what();
                    // s = "pair of " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + " not found ";
                    // s += std::to_string(idx_nb.first) + "," + std::to_string(idx_nb.second) + "\n";
                    // std::cout << s;
                    btrees_bag.insert_task(btt);
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
                btrees_bag.insert_task(btt);   
            }
            this->busy = false;
        }
    // }
    if(verbose) std::cout << "end worker search pair\n";
}
