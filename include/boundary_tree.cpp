#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, int64_t boundary_parent,int64_t boundary_levelroot){

    this->gval=gval;
    //this->in_tree=in_tree;
    this->maxtree_idx=maxtree_idx;
    //this->boundary_idx=boundary_idx;
    this->boundary_parent=boundary_parent;
    this->boundary_levelroot=boundary_levelroot;
}

boundary_node::boundary_node(maxtree_node *n, int64_t boundary_parent,int64_t boundary_levelroot){

    this->gval=n->gval;
    //this->in_tree=in_tree;
    this->maxtree_idx=n->idx;
    //this->boundary_idx=boundary_idx;
    this->boundary_parent=boundary_parent;
    this->boundary_levelroot=boundary_levelroot;
}

boundary_tree::boundary_tree(){
    this->border_elements=new std::unordered_map<uint64_t, boundary_node *>();
}

boundary_tree::boundary_tree(std::unordered_map<uint64_t, boundary_node *> *border_elements){
    this->border_elements=border_elements;
}

boundary_tree::~boundary_tree(){
    for(auto pairs: *this->border_elements){
        uint64_t n=pairs.first;
        delete pairs.second;
    }
    delete this->border_elements;
}
bool boundary_tree::insert_element(boundary_node &n){
    boundary_node *new_n;
    if(this->border_elements->find(n.maxtree_idx) != this->border_elements->end()){
        return false;
    }else{
        new_n = new boundary_node(n);
        this->border_elements->emplace(n.maxtree_idx, new_n);
        return true;
    }
}

std::string boundary_tree::to_string(enum boundary_tree_field f){
    
    uint32_t i,j;
    std::ostringstream ss;
    for(auto pairs: *(this->border_elements)){
        auto bn = pairs.second;
        if(f == BOUNDARY_PARENT){
            ss << bn->boundary_parent;
        }else if(f == BOUNDARY_LEVELROOT){
            ss << bn->boundary_levelroot;
        }else if(f == BOUNDARY_IDX){
            //ss << bn->boundary_idx;
        }else if(f == MAXTREE_IDX){
            ss << bn->maxtree_idx;
        }else if(f == BOUNDARY_GVAL){
            ss << bn->gval;
        }
        ss << " ";
    }
    return ss.str();
}