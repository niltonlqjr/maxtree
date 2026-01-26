#include <unordered_map>
#include <string>

#include "src/hps.h"

#include "const_enum_define.hpp"
#include "bag_of_task.hpp"

#ifndef __WORKERS_HPP__
#define __WORKERS_HPP__

class worker{
    private:
        uint16_t id;
        std::unordered_map<std::string, double> *attr;
    public:
        worker(uint16_t id, std::unordered_map<std::string, double> *attr);
        worker();
        // ~worker();
        void set_attr(std::string, double val);
        //virtual void run() = 0;
        Tprocess_power get_process_power();

        bool operator<(worker &r);
        bool operator>(worker &r);
        bool operator==(worker &r);
        
        void print();

        template <class B>
        void serialize(B &buf) const{
            buf << (*(this->attr)) << this->id;
        }

        template <class B>
        void parse(B &buf){
            buf >> (*(this->attr)) >> this->id;
        }
        
};

#endif