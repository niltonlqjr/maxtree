#include "maxtree_node.hpp"
#include <string>


maxtree_node::maxtree_node(double g, unsigned int i, double v){
    this->parent = -1;
    this->idx = i;
    this->label = -1;
    this->gval = g;
}

/* std::string maxtree_node::to_str(){
    std::string s;
    s = "(id:"+ std::to_string(this->idx) 
        +", parent:"+ std::to_string(this->parent) 
        +", gval:"+std::to_string(this->gval)+")";
    return s;
} */

bool maxtree_node::operator>(const maxtree_node &r){
    return this->gval > r.gval;
}
bool maxtree_node::operator>=(const maxtree_node &r){
    return this->gval >= r.gval;
}
bool maxtree_node::operator<(const maxtree_node &r){
    return this->gval < r.gval;
}
bool maxtree_node::operator<=(const maxtree_node &r){
    return this->gval <= r.gval;
}
bool maxtree_node::operator==(const maxtree_node &r){
    return this->gval == r.gval;
}
bool maxtree_node::operator!=(const maxtree_node &r){
    return this->gval != r.gval;
}