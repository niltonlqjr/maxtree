#include <cinttypes>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <functional>


#ifndef __BAG_OF_TASK_HPP__
#define __BAG_OF_TASK_HPP__

#include "heap.hpp"

template <class Task>
class bag_of_tasks{
    protected:
        std::deque<Task> *tasks;
        std::mutex lock;
        std::condition_variable has_task, no_task;
        int num_task;
        bool running;
        int waiting;
        void wakeup_workers();
    public:
        bag_of_tasks(bool start_running = false);
        ~bag_of_tasks();
        void start();
        void insert_task(Task t);
        int position_of(int priority);
        bool is_running();
        bool get_task(Task &ret);
        bool get_task_by_position(Task &ret, size_t position);
        template <class T> bool get_task_by_function(Task &ret, T value, T function(Task));
        Task at(int pos);
        void wait_empty();
        void notify_end();
        int num_waiting();
        int size();
        void print();
        bool empty();
        template <class T> uint64_t search_by_function(T value, T function(Task));
};


template <class Task>
class prio_bag_of_tasks: public bag_of_tasks<Task> {
    protected:
        min_heap<Task> *tasks;
    public:
        prio_bag_of_tasks(bool start_running = false);
        ~prio_bag_of_tasks();
        void insert_task(Task t);

        bool get_task(Task &ret, int priority = -1);
};


template <class Task, bool CompareLesser(Task, Task)>
class ordered_bag_of_tasks: public bag_of_tasks<Task>{
    protected:
        std::deque<Task> *tasks;
        // bool (*compare)(Task, Task);
    public:
        ordered_bag_of_tasks(bool start_running = false);
        ~ordered_bag_of_tasks();
        void insert_task(Task t);

};

#include "bag_of_task.tpp"

#endif