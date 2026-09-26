#include <unordered_map>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include "zmq.hpp"

#include "heap.hpp"
#include "connection.hpp"

#ifndef __SCHEDULER_OF_WORKERS_HPP__
#define __SCHEDULER_OF_WORKERS_HPP__


/* Worker class must have the operators < (lower than), > (greater than) and == (equal to) */
template <class Worker>
class scheduler_of_workers{
    protected:
        void wait_worker(std::unique_lock<std::mutex> &l);
        std::mutex lock;
        std::condition_variable cv;
        std::deque<Worker> workers;
        zmq::socket_t sock_send, sock_recv;
        connection c;
        
    public:
        scheduler_of_workers();
        scheduler_of_workers(zmq::context_t &context, std::string address_recv, std::string address_send);
        void insert_worker(Worker w);
        Worker get_worker();
        template <class T> size_t search_worker_by_function(T value, T function(Worker));
        void finish_worker(Worker w);
        void clear();
        Worker at(size_t i);
        size_t size();
        bool empty();
        void bind_sockets(std::string self_address_recv, std::string self_address_send);
        void connect();
        void disconnect();
        
        /* The worker must have a method get_index that returns a value which can be argument of to_string function*/
        void send_msg(std::string msg);

        std::pair<std::string, std::string> recv_msg();
        
};


template <class Worker, bool CompareLesser(Worker, Worker)>
class ordered_scheduler_of_workers : public scheduler_of_workers<Worker>{
    // protected:
        
    public:
        ordered_scheduler_of_workers();
        void insert_worker(Worker w);
        Worker get_worker();
};

template <class Type_idx, class Worker>
class hash_scheduler_of_worker{
    protected:
        std::mutex lock;
        std::condition_variable cv;
        std::unordered_map<Type_idx, Worker> workers;
        void wait_worker(std::unique_lock<std::mutex> &l);
    public:
        hash_scheduler_of_worker();
        void insert_worker(Type_idx idx, Worker w);
        Worker search_worker_by_idx(Type_idx idx);
        Worker get_worker(Type_idx idx);
        size_t size();
        bool empty();
        bool has_worker_key(Type_idx idx);

        


};


#include "scheduler_of_workers.tpp"

#endif


