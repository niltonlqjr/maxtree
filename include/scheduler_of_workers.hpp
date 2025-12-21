#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include "heap.hpp"

#ifndef __SCHEDULER_OF_WORKERS_HPP__
#define __SCHEDULER_OF_WORKERS_HPP__


/* Worker class must have the operators < (lower than), > (greater than) and == (equal to) */
template <class Worker>
class scheduler_of_workers{
    private:
        std::mutex lock;
        std::condition_variable cv;
        max_heap<Worker> *workers;
        uint64_t total_workers;
        uint64_t free_workers;
    public:
        scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker();
        void finish_worker(Worker w);    
};



#include "scheduler_of_workers.tpp"

#endif