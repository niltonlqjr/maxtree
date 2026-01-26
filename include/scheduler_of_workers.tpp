template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(){
    this->workers = new max_heap<Worker>();
    this->free_workers = 0;
    this->total_workers = 0;
}

template <class Worker>
void scheduler_of_workers<Worker>::insert_worker(Worker w){
    std::lock_guard(this->lock);
    this->workers->insert(w);
    this->total_workers++;
    this->free_workers++;
}

/* 
return the best worker at its registered workers
if this scheduler has no workers, it throws std::length_error
if all workers are busy, it throws std::range_error
*/
template <class Worker>
Worker scheduler_of_workers<Worker>::get_best_worker(){
    std::lock_guard(this->lock);
    if(this->total_workers == 0){
        throw std::length_error("total workers equals 0");
    }
    if(this->free_workers > 0){
        Worker r = this->workers->at(0);
        this->workers->remove_at(0);
        this->free_workers--;
        return r;
    }else{
        throw std::range_error("not free workers");
    }
    
}

template <class Worker>
void scheduler_of_workers<Worker>::finish_worker(Worker w){
    for(int64_t i=0; i < this->workers->size(); i++){
        auto worker = this->at(i);
        if(worker == w){
            this->workers->remove_at(i);
        }
    }
}

template <class Worker>
Worker scheduler_of_workers<Worker>::at(size_t i){
    return this->workers->at(i);
}

template <class Worker>
size_t scheduler_of_workers<Worker>::size(){
    return this->total_workers;
}