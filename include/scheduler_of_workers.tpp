#include "scheduler_of_workers.hpp"


template <class Worker, class Type_idx>
scheduler_of_workers<Worker, Type_idx>::scheduler_of_workers()
{
    // this->free_workers = new max_heap<Worker>();
    this->bindable = false;
}

template <class Worker, class Type_idx>
scheduler_of_workers<Worker, Type_idx>::scheduler_of_workers(zmq::context_t &context, std::string address_recv, std::string address_send)
{
    // this->free_workers = new max_heap<Worker>();
    this->c = connection(context, address_send, address_recv, zmq::socket_type::router);
    this->bindable = true;
}

template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::set_info(TConnectionIdx cid, zmq::context_t &context, std::string address_recv, std::string address_send){
    this->c = connection(cid, context, address_recv, address_send, zmq::socket_type::router);
    this->bindable = true;
}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->free_workers.insert(w);
    this->free_workers.push_back(w);
    this->cv.notify_all();
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/

// template <class Worker>
template <class Worker, class Type_idx>
Worker scheduler_of_workers<Worker, Type_idx>::get_free_worker(){
    std::unique_lock<std::mutex> l(this->lock);
    // this->wait_worker(l);
    while(this->free_workers.size() <= 0){
        this->cv.wait(l);
    }

    Worker r = this->free_workers.at(0);
    // Worker r = this->free_workers.front();
    this->free_workers.pop_front();
    
    return r;
    
}

template <class Worker, class Type_idx>
inline Worker scheduler_of_workers<Worker, Type_idx>::get_busy_worker_by_idx(Type_idx idx){
    Worker w = this->busy_workers.at(idx);
    return w;
}

template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::set_busy_worker(Type_idx idx, Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    this->busy_workers[idx] = w;
}

// template <class Worker>
template <class Worker, class Type_idx>
template <class T>
size_t scheduler_of_workers<Worker, Type_idx>::search_free_worker_by_function(T value, T function(Worker)){
    std::unique_lock<std::mutex> l(this->lock);
    // this->wait_worker(l);
    while(this->free_workers.size() <= 0){
        this->cv.wait(l);
    }
    for(size_t i=0; i < this->free_workers.size(); i++){
        if(function(this->free_workers.at(i)) == value){
            return i;
        }
    }
    throw std::out_of_range("scheduler_of_workers<Worker, Type_idx>::search_worker_by_function --- Worker not found");
}

// template<class Worker>
template <class Worker, class Type_idx>
inline void scheduler_of_workers<Worker, Type_idx>::wait_worker(std::unique_lock<std::mutex>  &l){
    while(this->free_workers.empty()){
        std::cout << "waiting_worker\n";
        this->cv.wait(l);
    }
}

// template <class Worker>
template <class Worker, class Type_idx> 
void scheduler_of_workers<Worker, Type_idx>::finish_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    for(int64_t i=0; i < this->free_workers.size(); i++){
        Worker worker;
        try{
            worker = this->free_workers.at(i);
        }catch(...){
            throw std::out_of_range("scheduler_of_workers<Worker, Type_idx>::finish_worker --- Worker not found");
        }
        if(worker == w){
            this->free_workers.remove_at(i);
        }
    }
}

// template <class Worker>
template <class Worker, class Type_idx>
inline void scheduler_of_workers<Worker, Type_idx>::clear(){
    std::unique_lock<std::mutex> l(this->lock);
    this->free_workers.clear();
}

// template <class Worker>
template <class Worker, class Type_idx>
Worker scheduler_of_workers<Worker, Type_idx>::at(size_t i){
    std::unique_lock<std::mutex> l(this->lock);
    Worker ret;
    try{
        ret = this->free_workers.at(i);
    }catch(...){
        throw std::out_of_range("scheduler_of_workers<Worker, Type_idx>::at --- Worker not found");
    }
    return ret;
}

// template <class Worker>
template <class Worker, class Type_idx>
size_t scheduler_of_workers<Worker, Type_idx>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.size();
}

// template <class Worker>
template <class Worker, class Type_idx>
bool scheduler_of_workers<Worker, Type_idx>::empty(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.size() == 0;
}



// template <class Worker>
template <class Worker, class Type_idx>
std::pair<std::string, std::string> scheduler_of_workers<Worker, Type_idx>::recv_msg(){
    // return this->c.recv_message();
    // return std::pair<std::string, std::string>();
    std::unique_lock<std::mutex> l(this->lock_recv);
    while(this->recv_msg_queue.empty()){
        this->cv_recv.wait(l);
    }
    std::pair<std::string, std::string> ret = this->recv_msg_queue.front();
    this->recv_msg_queue.pop();
    return ret;
}

// template <class Worker>
template <class Worker, class Type_idx>
inline void scheduler_of_workers<Worker, Type_idx>::loop_recv(){
    //put messages on recv_msg_queue
    while(this->running){
        std::pair<std::string, std::string> msg = this->c.recv_message();
        {
            std::unique_lock<std::mutex> l(this->lock_recv);
            this->recv_msg_queue.push(msg);
            this->cv_recv.notify_all();
        }
    }
}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::send_msg(std::string msg){
    // Worker w = this->get_free_worker();
    std::unique_lock<std::mutex> l(this->lock_send);
    
    // this->c.send_message(worker_id, msg);
    this->send_msg_queue.push(std::string(msg));
    this->cv_send.notify_all();
}

// template <class Worker>
template <class Worker, class Type_idx>
inline void scheduler_of_workers<Worker, Type_idx>::loop_send(){
    std::string msg, worker_id;
    Worker w;
    while(this->running){
        {
            std::unique_lock<std::mutex> l(this->lock_send);
            while(this->send_msg_queue.empty()){
                this->cv_send.wait(l);
            }
            using std::to_string;
            msg = this->send_msg_queue.front();
            this->send_msg_queue.pop();
        }
        w = this->get_free_worker();
        worker_id = to_string(w.get_index());

        this->c.send_message(worker_id, msg);
    }
}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::run(){
    //std::cerr << "NOT IMPLEMENTED YET\n";
    // //  create a thread for loop_recv and loop_send
    // this->thread_recv = std::thread(&scheduler_of_workers<Worker, Type_idx>::loop_recv, this);
    // this->thread_send = std::thread(&scheduler_of_workers<Worker, Type_idx>::loop_send, this);
    this->thread_recv = std::thread(&scheduler_of_workers<Worker, Type_idx>::loop_recv,this);
    this->thread_send = std::thread(&scheduler_of_workers<Worker, Type_idx>::loop_send,this);
    this->running = true;
    return;

}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::stop(){
    this->running = false;

}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::bind_sockets(){
    
    // std::string self_address_recv = protocol+"://*:"+port_recv;
    // this->sock_recv.bind(address_recv);
    
    // std::string self_address_send = protocol+"://*:"+port_send;
    // this->sock_send.bind(address_send);
    if(this->bindable){
        this->c.bind();
    }else{
        std::cerr << "impossible to bind sockets of scheduler\n";
        exit(EXIT_FAILURE);
    }
}


// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::connect(){
    // this->sock_recv.connect(address_recv);
    // this->sock_send.connect(address_send);
    this->c.connect();

}

// template <class Worker>
template <class Worker, class Type_idx>
void scheduler_of_workers<Worker, Type_idx>::disconnect(){
    this->c.disconnect();
}

template <class Worker, class Type_idx>
inline void scheduler_of_workers<Worker, Type_idx>::close_sockets(){
    this->c.close_sockets();
}

/*==============================================================================================================
  ====================================     ordered_scheduler_of_workers     ====================================
  ==============================================================================================================*/


template <class Worker, bool CompareLesser(Worker, Worker)>
ordered_scheduler_of_workers<Worker, CompareLesser>::ordered_scheduler_of_workers(){
    // this->free_workers = new max_heap<Worker>();
}


template <class Worker, bool CompareLesser(Worker, Worker)>
void ordered_scheduler_of_workers<Worker, CompareLesser>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->free_workers.insert(w);
    this->free_workers.push_back(w);
    size_t i=this->free_workers.size()-1;
    while(i > 0 && CompareLesser(this->free_workers.at(i-1), w)){
        this->free_workers.at(i) = this->free_workers.at(i-1);
        i--;
    }
    this->cv.notify_all();
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/

template <class Worker, bool CompareLesser(Worker, Worker)>
Worker ordered_scheduler_of_workers<Worker, CompareLesser>::get_worker(){
    std::unique_lock<std::mutex> l(this->lock);
    // this->wait_worker(l);
    while(this->free_workers.size() <= 0){
        this->cv.wait(l);
    }
    Worker r = this->free_workers.at(0);
    // Worker r = this->free_workers.front();
    this->free_workers.pop_front();
    return r;
    
}

/*==============================================================================================================
  ====================================     hash_scheduler_of_workers     ====================================
  ==============================================================================================================*/


template <class Type_idx, class Worker>
inline hash_scheduler_of_worker<Type_idx, Worker>::hash_scheduler_of_worker(){
}
template <class Type_idx, class Worker>
inline void hash_scheduler_of_worker<Type_idx, Worker>::insert_worker(Type_idx idx, Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // std::string _s= "++++++++> inserting worker " + std::to_string(w->get_index()) + "\n";
    // std::cout << _s;
    this->free_workers[idx] = w; // this->free_workers.insert(idx, w);

}

template <class Type_idx, class Worker>
inline Worker hash_scheduler_of_worker<Type_idx, Worker>::search_worker_by_idx(Type_idx idx){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.at(idx);
}

template <class Type_idx, class Worker>
inline void hash_scheduler_of_worker<Type_idx, Worker>::wait_worker(std::unique_lock<std::mutex> &l){
    while(this->free_workers.size() <= 0){
        this->cv.wait(l);
    }

}

template <class Type_idx, class Worker>
inline size_t hash_scheduler_of_worker<Type_idx, Worker>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    
    return this->free_workers.size();
}

template <class Type_idx, class Worker>
inline Worker hash_scheduler_of_worker<Type_idx, Worker>::get_worker(Type_idx idx){
    std::unique_lock<std::mutex> l(this->lock);
    // this->wait_worker(l);

    while(this->free_workers.size() <= 0){
        this->cv.wait(l);
    }

    Worker ret = this->free_workers.at(idx);
    this->free_workers.erase(idx);
    // std::string _s= "--------> removing worker " + std::to_string(ret->get_index()) + "\n";
    // std::cout << _s;
    return ret;
}


template <class Type_idx, class Worker>
inline bool hash_scheduler_of_worker<Type_idx, Worker>::empty(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.size() == 0;
}

template <class Type_idx, class Worker>
inline bool hash_scheduler_of_worker<Type_idx, Worker>::has_worker_key(Type_idx idx){
    return this->free_workers.find(idx) != this->free_workers.end();
}
