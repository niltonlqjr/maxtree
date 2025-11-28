#include <unordered_map>
#include <string>

#include "bag_of_task.hpp"

#ifndef __WORKER_HPP__
#define __WORKER_HPP__

template <class Task>
class worker{
    private:
        std::unordered_map<std::string, double> attr;
    public:
        worker(std::unordered_map<std::string, double> attr);
        worker();


}

#include "worker.tpp"

#endif