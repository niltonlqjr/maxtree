#include "workers.hpp"

worker::worker(std::unordered_map<std::string, double> *attr){
    this->attr = attr;
}

worker::worker(){
    this->attr = new std::unordered_map<std::string, double>();
}

worker::~worker(){
    delete this->attr;
}

void worker::set_attr(std::string attr_name, double attr_val){
    (*this->attr)[attr_name] = attr_val;
}

Tprocess_power worker::get_process_power(){
    return 1;//this->attr->at("MHZ") * this->attr->at("NUMPROC");
}

