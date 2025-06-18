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

boundary_tree::boundary_tree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h=h;
    this->w=w;
    this->grid_i=grid_i;
    this->grid_j = grid_j;
    this->border_elements=new std::vector<std::unordered_map<uint64_t, boundary_node *>*>();
    for(auto b : TBordersVector){
        this->border_elements->push_back(new std::unordered_map<uint64_t, boundary_node *>());
    }
}

boundary_tree::boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
                             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h=h;
    this->w=w;
    this->grid_i=grid_i;
    this->grid_j = grid_j;
    this->border_elements=border_elements;
}

boundary_tree::~boundary_tree(){
    for(uint32_t i=0;i < this->border_elements->size(); i++){
        for(auto pairs: *this->border_elements->at(i)){
            uint64_t n=pairs.first;
            delete pairs.second;
        }
        delete this->border_elements->at(i);
    }
    delete this->border_elements;
}

bool boundary_tree::insert_element(boundary_node &n, enum borders b, int64_t origin){
    boundary_node *new_n;
    auto border = this->border_elements->at(b);
    if(border->find(n.maxtree_idx) != border->end()){
        return false;
    }else{
        new_n = new boundary_node(n);
        border->emplace(n.maxtree_idx, new_n);
        return true;
    }
}

void boundary_tree::add_parents(maxtree_node *tn, enum borders b, std::vector<maxtree_node*> *maxtree_data){
    maxtree_node *parent;
    boundary_node *current;
    int64_t parent_idx;
    int64_t pidx;
    current=this->get_border_node(tn->idx, b);//get the added node
    while(current!=NULL){
        pidx = maxtree_data->at(current->maxtree_idx)->parent; // get parent idx of current boundary node
        if(pidx >= 0){// if this node has a parent (not the tile root)
            parent = maxtree_data->at(pidx);
            boundary_node bound_parent(parent,current->maxtree_idx); //create the parent node to add on bondary tree
            this->insert_element(bound_parent,b);
        }else{ 
            pidx = -1;
        }
        current->boundary_parent = pidx; // update the parent of current node
        current=this->get_border_node(pidx,b); // go to the parent and add its ancerstors
    }
}

boundary_node *boundary_tree::get_border_node(int64_t maxtree_idx, enum borders b){
    boundary_node *ret;
    auto border = this->border_elements->at(b);
    if (maxtree_idx >= 0){
        ret = border->at(maxtree_idx);
    }else{
        ret = NULL;
    }
    return ret;
}

void boundary_tree::merge(boundary_tree *t, enum merge_directions d){
    std::unordered_map<uint64_t, boundary_node *> *v_this, *v_t;
    if(d == MERGE_HORIZONTAL){
        if(this->grid_i < t->grid_i){
            v_this = this->border_elements->at(BOTTOM_BORDER);
            v_t = t->border_elements->at(TOP_BORDER);
        }else{
            v_this = this->border_elements->at(TOP_BORDER);
            v_t = t->border_elements->at(BOTTOM_BORDER);
        }
    }else if(d == MERGE_VERTICAL){
        if(this->grid_j < t->grid_j){
            v_this = this->border_elements->at(RIGHT_BORDER);
            v_t = t->border_elements->at(LEFT_BORDER);
        }else{
            v_this = this->border_elements->at(LEFT_BORDER);
            v_t = t->border_elements->at(RIGHT_BORDER);
        }
    }

    

    std::cout << "merge nodes:\n";
    for(auto x: *v_this){
        std::cout << x.first <<" ";
    }
    std::cout << "\n";
    for(auto x: *v_t){
        std::cout << x.first <<" ";
    }
    std::cout << "\n";
    
    if(d==MERGE_VERTICAL){
        if(this->grid_i < t->grid_i){
            this->grid_i = t->grid_i;
        }else{
            t->grid_i = this->grid_i;
        }
    }else if(d==MERGE_VERTICAL){
        if(this->grid_j < t->grid_j){
            this->grid_j = t->grid_j;
        }else{
            t->grid_j = this->grid_j;
        }
    }

}

std::string boundary_tree::to_string(enum boundary_tree_field f){
    
    uint32_t i,j;
    std::ostringstream ss;
    for(int i=0; i<NamesBordersVector.size();i++){
        ss << NamesBordersVector[i] << ":";
        auto v = *(this->border_elements->at(i));
        for(auto pairs: v){
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
        ss << "\n";
    }
    return ss.str();
}