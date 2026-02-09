#include <unordered_map>
#include <vector>
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
        std::vector<Worker *> workers_ptr;
    public:
        scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker();
        void wait_free_worker();
        void finish_worker(Worker w);
        Worker at(size_t i);
        size_t size();
};



#include "scheduler_of_workers.tpp"

#endif