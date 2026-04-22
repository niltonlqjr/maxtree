#include "scheduler_of_workers.hpp"
template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(){
    // this->workers = new max_heap<Worker>();
    
}

template <class Worker>
void scheduler_of_workers<Worker>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->workers.insert(w);
    this->workers.push_back(w);
    this->cv.notify_all();
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/

template <class Worker>
Worker scheduler_of_workers<Worker>::get_worker(){
    std::unique_lock<std::mutex> l(this->lock);
    this->wait_worker(l);
    
    // Worker r = this->workers.at(0);
    Worker r = this->workers.back();
    this->workers.pop_back();
    return r;
    
}


template <class Worker>
template <class T>
size_t scheduler_of_workers<Worker>::search_worker_by_function(T value, T function(Worker)){
    std::unique_lock<std::mutex> l(this->lock);
    this->wait_worker(l);
    for(size_t i=0; i < this->workers.size(); i++){
        if(function(this->workers.at(i)) == value){
            return i;
        }
    }
    throw std::out_of_range("scheduler_of_workers<Worker>::search_worker_by_function --- Worker not found");
}


template<class Worker>
inline void scheduler_of_workers<Worker>::wait_worker(std::unique_lock<std::mutex>  &l){
    while(this->workers.size() <= 0){
        this->cv.wait(l);
    }
}

template <class Worker>
void scheduler_of_workers<Worker>::finish_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    for(int64_t i=0; i < this->workers.size(); i++){
        Worker worker;
        try{
            worker = this->workers.at(i);
        }catch(...){
            throw std::out_of_range("scheduler_of_workers<Worker>::finish_worker --- Worker not found");
        }
        if(worker == w){
            this->workers.remove_at(i);
        }
    }
}

template <class Worker>
Worker scheduler_of_workers<Worker>::at(size_t i){
    std::unique_lock<std::mutex> l(this->lock);
    Worker ret;
    try{
        ret = this->workers.at(i);
    }catch(...){
        throw std::out_of_range("scheduler_of_workers<Worker>::at --- Worker not found");
    }
    return ret;
}

template <class Worker>
size_t scheduler_of_workers<Worker>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->workers.size();
}

/*==============================================================================================================
  ====================================     ordered_scheduler_of_workers     ====================================
  ==============================================================================================================*/


template <class Worker, bool CompareLesser(Worker, Worker)>
inline ordered_scheduler_of_workers<Worker, CompareLesser>::ordered_scheduler_of_workers(){
    // this->workers = new max_heap<Worker>();
}

template <class Worker, bool CompareLesser(Worker, Worker)>
void ordered_scheduler_of_workers<Worker, CompareLesser>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->workers.insert(w);
    this->workers.push_back(w);
    size_t i=this->workers.size()-1;
    while(i > 0 && CompareLesser(this->workers.at(i-1), w)){
        this->workers.at(i) = this->workers.at(i-1);
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
    this->wait_worker(l);
    // Worker r = this->workers.at(0);
    Worker r = this->workers.front();
    this->workers.pop_front();
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
    this->workers[idx] = w; // this->workers.insert(idx, w);

}

template <class Type_idx, class Worker>
inline Worker hash_scheduler_of_worker<Type_idx, Worker>::search_worker_by_idx(Type_idx idx){
    std::unique_lock<std::mutex> l(this->lock);
    return this->workers.at(idx);
}

template <class Type_idx, class Worker>
inline void hash_scheduler_of_worker<Type_idx, Worker>::wait_worker(std::unique_lock<std::mutex> &l){
    while(this->workers.size() <= 0){
        this->cv.wait(l);
    }

}

template <class Type_idx, class Worker>
inline size_t hash_scheduler_of_worker<Type_idx, Worker>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    
    return this->workers.size();
}

template <class Type_idx, class Worker>
inline Worker hash_scheduler_of_worker<Type_idx, Worker>::get_worker(Type_idx idx){
    std::unique_lock<std::mutex> l(this->lock);
    this->wait_worker(l);
    Worker ret = this->workers.at(idx);
    this->workers.erase(idx);
    // std::string _s= "--------> removing worker " + std::to_string(ret->get_index()) + "\n";
    // std::cout << _s;
    return ret;
}