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
    protected:
        std::mutex lock;
        std::condition_variable cv;
        std::deque<Worker> *workers;
        uint64_t total_workers;
        uint64_t free_workers;
    public:
        scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker(bool wait_at_least_one=false);
        template <class T> size_t search_worker_by_function(T value, T function(Worker));
        void wait_free_worker();
        void finish_worker(Worker w);
        Worker at(size_t i);
        size_t size();
};


template <class Worker, bool CompareLesser(Worker, Worker)>
class ordered_scheduler_of_workers : public scheduler_of_workers<Worker>{
    public:
        ordered_scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_best_worker(bool wait_at_least_one=false);
};


#include "scheduler_of_workers.tpp"

#endif