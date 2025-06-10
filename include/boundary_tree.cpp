#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t boundary_idx, bool in_tree,
                             int64_t boundary_parent,int64_t boundary_levelroot){

    this->gval=gval;
    this->in_tree=in_tree;
    this->maxtree_idx=maxtree_idx;
    this->boundary_idx=boundary_idx;
    this->boundary_parent=boundary_parent;
    this->boundary_levelroot=boundary_levelroot;
}

boundary_node::boundary_node(maxtree_node *n, uint64_t boundary_idx, bool in_tree,
                             int64_t boundary_parent,int64_t boundary_levelroot){

    this->gval=n->gval;
    this->in_tree=in_tree;
    this->maxtree_idx=n->idx;
    this->boundary_idx=boundary_idx;
    this->boundary_parent=boundary_parent;
    this->boundary_levelroot=boundary_levelroot;
}

boundary_tree::boundary_tree(){
    this->border_elements=new std::vector<boundary_node *>();
}

boundary_tree::boundary_tree(std::vector<boundary_node *> *border_elements){
    this->border_elements=border_elements;
}

boundary_tree::~boundary_tree(){
    for(boundary_node *n: *this->border_elements){
        delete n;
    }
    delete border_elements;
}
void boundary_tree::insert_element(boundary_node n){
    this->border_elements->push_back(new boundary_node(n));
}