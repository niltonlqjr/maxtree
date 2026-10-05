#include "scheduler_of_workers.hpp"

template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(){
    // this->free_workers = new max_heap<Worker>();
    this->bindable = false;
}

template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(zmq::context_t &context, std::string address_recv, std::string address_send)
{
    // this->free_workers = new max_heap<Worker>();
    this->c = connection(context, address_send, address_recv, zmq::socket_type::router);
    this->bindable = true;
}

template <class Worker>
void scheduler_of_workers<Worker>::set_info(zmq::context_t &context, std::string address_recv, std::string address_send){
    this->c = connection(context, address_recv, address_send, zmq::socket_type::router);
    this->bindable = true;
}

template <class Worker>
void scheduler_of_workers<Worker>::insert_worker(Worker w){
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

template <class Worker>
Worker scheduler_of_workers<Worker>::get_free_worker(){
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


template <class Worker>
template <class T>
size_t scheduler_of_workers<Worker>::search_free_worker_by_function(T value, T function(Worker)){
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
    throw std::out_of_range("scheduler_of_workers<Worker>::search_worker_by_function --- Worker not found");
}

template<class Worker>
inline void scheduler_of_workers<Worker>::wait_worker(std::unique_lock<std::mutex>  &l){
    while(this->free_workers.empty()){
        std::cout << "waiting_worker\n";
        this->cv.wait(l);
    }
}

template <class Worker>
void scheduler_of_workers<Worker>::finish_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    for(int64_t i=0; i < this->free_workers.size(); i++){
        Worker worker;
        try{
            worker = this->free_workers.at(i);
        }catch(...){
            throw std::out_of_range("scheduler_of_workers<Worker>::finish_worker --- Worker not found");
        }
        if(worker == w){
            this->free_workers.remove_at(i);
        }
    }
}

template <class Worker>
inline void scheduler_of_workers<Worker>::clear(){
    std::unique_lock<std::mutex> l(this->lock);
    this->free_workers.clear();
}

template <class Worker>
Worker scheduler_of_workers<Worker>::at(size_t i){
    std::unique_lock<std::mutex> l(this->lock);
    Worker ret;
    try{
        ret = this->free_workers.at(i);
    }catch(...){
        throw std::out_of_range("scheduler_of_workers<Worker>::at --- Worker not found");
    }
    return ret;
}

template <class Worker>
size_t scheduler_of_workers<Worker>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.size();
}

template <class Worker>
bool scheduler_of_workers<Worker>::empty(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->free_workers.size() == 0;
}

template <class Worker>
void scheduler_of_workers<Worker>::send_msg(std::string msg){
    Worker w = this->get_free_worker();
    using std::to_string;
    std::string worker_id = to_string(w.get_index());
    this->c.send_message(worker_id, msg);
}

template <class Worker>
std::pair<std::string, std::string> scheduler_of_workers<Worker>::recv_msg(){
    return this->c.recv_message();
    return std::pair<std::string, std::string>();
}

template <class Worker>
inline void scheduler_of_workers<Worker>::run(){
    std::cerr << "NOT IMPLEMENTED YET\n";
    return;
    // while(true){
    //     this->recv_msg();
    // }
}

template <class Worker>
void scheduler_of_workers<Worker>::bind_sockets(){
    
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


template <class Worker>
void scheduler_of_workers<Worker>::connect(){
    // this->sock_recv.connect(address_recv);
    // this->sock_send.connect(address_send);
    this->c.connect();

}

template <class Worker>
void scheduler_of_workers<Worker>::disconnect(){
    this->c.disconnect();
}

template <class Worker>
inline void scheduler_of_workers<Worker>::close_sockets(){
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
