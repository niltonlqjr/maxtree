#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin, int64_t boundary_parent, int64_t boundary_levelroot){
    this->gval=gval;
    //this->in_tree=in_tree;
    this->maxtree_idx=maxtree_idx;
    this->origin = origin;
    //this->boundary_idx=boundary_idx;
    this->boundary_parent=boundary_parent;
    this->boundary_levelroot=boundary_levelroot;
}

boundary_node::boundary_node(maxtree_node *n, uint64_t origin, int64_t boundary_parent,int64_t boundary_levelroot){
    this->gval=n->gval;
    //this->in_tree=in_tree;
    this->maxtree_idx=n->idx;
    this->origin = origin;
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

bool boundary_tree::insert_element(boundary_node &n, int64_t origin){
    boundary_node *new_n;
    if(this->border_elements->find(n.maxtree_idx) != this->border_elements->end()){
        return false;
    }else{
        new_n = new boundary_node(n);
        this->border_elements->emplace(n.maxtree_idx, new_n);
        return true;
    }
}

void boundary_tree::add_parents(maxtree_node *tn, std::vector<maxtree_node*> *maxtree_data){
    maxtree_node *parent;
    boundary_node *current;
    int64_t parent_idx;
    int64_t pidx;
    current=this->get_border_node(tn->idx);//get the added node
    while(current!=NULL){
        pidx = maxtree_data->at(current->maxtree_idx)->parent; // get parent idx of current boundary node
        if(pidx >= 0){// if this node has a parent (not the tile root)
            parent = maxtree_data->at(pidx);
            boundary_node bound_parent(parent,tn->idx); //create the parent node to add on bondary tree
            this->insert_element(bound_parent);
        }else{ 
            pidx = -1;
        }
        current->boundary_parent = pidx; // update the parent of current node
        current=this->get_border_node(pidx); // go to the parent and add its ancerstors
    }
}

boundary_node *boundary_tree::get_border_node(int64_t maxtree_idx){
    boundary_node *ret;
    if (maxtree_idx >= 0){
        ret = this->border_elements->at(maxtree_idx);
    }else{
        ret = NULL;
    }
    return ret;
}

void boundary_tree::merge(boundary_tree *t){

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