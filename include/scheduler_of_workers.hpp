#include <unordered_map>

#include "heap.hpp"

#ifndef __SCHEDULER_OF_WORKERS_HPP__
#define __SCHEDULER_OF_WORKERS_HPP__


/* Worker class must have the operators < (lower than), > (greater than) and == (equal to) */
template <class Worker>
class scheduler_of_workers{
    private:
        max_heap<Worker> *workers;
        uint64_t total_workers;
        uint64_t free_workers;
    public:
        scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker();
        void finish_worker(Worker w);    
};


template <class Worker>
scheduler_of_workers<Worker>::scheduler_of_workers(){
    this->workers = new max_heap();
    this->free_workers = 0;
    this->total_workers = 0;
}

template <class Worker>
void scheduler_of_workers<Worker>::insert_worker(Worker w){
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
    if(this->total_workers == 0){
        throw std::length_error;
    }
    if(this->free_workers > 0){
        Worker r = this->workers->at(0);
        this->workers->remove_at(0);
        this->free_workers--;
    }else{
        throw std::range_error;
    }
    return r;
}

template <class Worker>
void scheduler_of_workers<Worker>::finish_worker(Worker w){
    
}


#include "queue_of_workers.tpp"

#endif