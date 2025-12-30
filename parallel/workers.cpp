#include "workers.hpp"

worker::worker(uint16_t id, std::unordered_map<std::string, double> *attr){
    if(attr == NULL || attr == nullptr){
        this->attr = new std::unordered_map<std::string, double>();
    }else{
        this->attr = attr;
    }
    this->id = id;
}

worker::worker(){
    this->attr = new std::unordered_map<std::string, double>();
    this->id = 0;
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

void worker::print(){
    std::cout << "local id:" << this->id << " -- ";
    std::cout << "attributes:\n";
    for(auto elem: *this->attr){
        std::cout << "(" << elem.first << "," << elem.second << ") ";
    }
    std::cout << "\n";

}

// template <class B>
// void worker::serialize(B &buf) const {
//     buf << (*(this->attr)) << this->id;
// }

// template <class B>
// void worker::parse(B &buf){
//     buf >> (*(this->attr)) >> this->id;
    
// }
