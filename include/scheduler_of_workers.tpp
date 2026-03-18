template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(){
    // this->workers = new max_heap<Worker>();
    this->workers = new std::deque<Worker>();
    this->free_workers = 0;
    this->total_workers = 0;
}

template <class Worker>
void scheduler_of_workers<Worker>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->workers->insert(w);
    this->workers->push_back(w);
    this->total_workers++;
    this->cv.notify_all();
    // this->free_workers++;
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/

template <class Worker>
Worker scheduler_of_workers<Worker>::get_best_worker(bool wait_at_least_one){
    if(wait_at_least_one){
        this->wait_free_worker();
    }
    std::unique_lock<std::mutex> l(this->lock);
    if(this->total_workers <= 0){
        throw std::length_error("total workers equals 0");
    }
    
    // Worker r = this->workers->at(0);
    Worker r = this->workers->back();
    this->workers->pop_back();
    // this->free_workers--;
    this->total_workers--;
    return r;
    
}


template <class Worker>
template <class T>
size_t scheduler_of_workers<Worker>::search_worker_by_function(T value, T function(Worker)){
    std::unique_lock<std::mutex> l(this->lock);
    for(size_t i=0; i < this->workers->size(); i++){
        if(function(this->workers->at(i)) == value){
            return i;
        }
    }
    throw std::out_of_range("Worker not found");
}


template<class Worker>
void scheduler_of_workers<Worker>::wait_free_worker(){
    std::unique_lock<std::mutex> l(this->lock);
    while(this->free_workers < 0){
        this->cv.wait(l);
    }
}

template <class Worker>
void scheduler_of_workers<Worker>::finish_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    for(int64_t i=0; i < this->workers->size(); i++){
        Worker worker;
        try{
            worker = this->workers->at(i);
        }catch(...){
            throw std::out_of_range("worker not found");
        }
        if(worker == w){
            this->workers->remove_at(i);
            this->total_workers--;
        }
    }
}

template <class Worker>
Worker scheduler_of_workers<Worker>::at(size_t i){
    std::unique_lock<std::mutex> l(this->lock);
    Worker ret;
    try{
        ret = this->workers->at(i);
    }catch(...){
        throw std::out_of_range("worker not found");
    }
    return ret;
}

template <class Worker>
size_t scheduler_of_workers<Worker>::size(){
    std::unique_lock<std::mutex> l(this->lock);
    return this->total_workers;
}

/*==============================================================================================================
  ====================================     ordered_scheduler_of_workers     ====================================
  ==============================================================================================================*/


template <class Worker, bool CompareLesser(Worker, Worker)>
ordered_scheduler_of_workers<Worker, CompareLesser>::ordered_scheduler_of_workers(){
    // this->workers = new max_heap<Worker>();
    this->workers = new std::deque<Worker>();
    this->free_workers = 0;
    this->total_workers = 0;
}

template <class Worker, bool CompareLesser(Worker, Worker)>
void ordered_scheduler_of_workers<Worker, CompareLesser>::insert_worker(Worker w){
    std::unique_lock<std::mutex> l(this->lock);
    // this->workers->insert(w);
    this->workers->push_back(w);
    size_t i=this->workers->size()-1;
    while(i > 0 && CompareLesser(this->workers->at(i-1), w)){
        this->workers->at(i) = this->workers->at(i-1);
        i--;
    }
    this->total_workers++;
    this->cv.notify_all();
    // this->free_workers++;
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/

template <class Worker, bool CompareLesser(Worker, Worker)>
Worker ordered_scheduler_of_workers<Worker, CompareLesser>::get_best_worker(bool wait_at_least_one){
    if(wait_at_least_one){
        this->wait_free_worker();
    }
    std::unique_lock<std::mutex> l(this->lock);
    if(this->total_workers <= 0){
        throw std::length_error("total workers equals 0");
    }
    
    // Worker r = this->workers->at(0);
    Worker r = this->workers->front();
    this->workers->pop_front();
    // this->free_workers--;
    this->total_workers--;
    return r;
    
}

