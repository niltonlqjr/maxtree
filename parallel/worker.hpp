#include <unordered_map>
#include <string>


#include "const_enum_define.hpp"
#include "bag_of_task.hpp"

#ifndef __WORKER_HPP__
#define __WORKER_HPP__

class worker{
    private:
        std::unordered_map<std::string, double> *attr;
        
    public:
        worker(std::unordered_map<std::string, double> *attr);
        worker();
        ~worker();
        void set_attr(std::string, double val);
        virtual void run() = 0;
        Tprocess_power get_process_power();
        
};

#endif