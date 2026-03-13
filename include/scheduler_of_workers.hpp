#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>

#include "heap.hpp"

#ifndef __SCHEDULER_OF_WORKERS_HPP__
#define __SCHEDULER_OF_WORKERS_HPP__


/* Worker class must have the operators < (lower than), > (greater than) and == (equal to) */
template <class Worker>
class scheduler_of_workers{
    private:
        std::mutex lock;
        std::condition_variable cv;
        // max_heap<Worker> *workers;
        std::vector<Worker> *workers;
        uint64_t total_workers;
        uint64_t free_workers;
        std::vector<Worker *> workers_ptr;
    public:
        scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker(bool wait_at_least_one=false);
        void wait_free_worker();
        void finish_worker(Worker w);
        Worker at(size_t i);
        size_t size();
};


template <class Worker>
class ordered_scheduler_of_workers{
    private:
        std::mutex lock;
        std::condition_variable cv;
        // max_heap<Worker> *workers;
        std::deque<Worker> *workers;
        uint64_t total_workers;
        uint64_t free_workers;
        
    public:
        ordered_scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker(bool wait_at_least_one=false);
        template <class T> size_t search_worker_by_function(T value, T function(Worker));
        void wait_free_worker();
        void finish_worker(Worker w);
        Worker at(size_t i);
        size_t size();
};


#include "scheduler_of_workers.tpp"

#endif