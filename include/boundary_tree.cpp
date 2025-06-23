#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin,
                             uint64_t global_idx, int64_t bound_parent){
    this->gval = gval;
    this->maxtree_idx = maxtree_idx;
    this->origin = origin;
    this->global_idx = global_idx;
    this->boundary_parent = bound_parent;

}

boundary_node::boundary_node(maxtree_node *n, uint64_t origin,
                             int64_t bound_parent){
    this->gval = n->gval;
    this->maxtree_idx = n->idx;
    this->origin = origin;
    this->global_idx = n->global_idx;
    this->boundary_parent = bound_parent;
    
}

boundary_tree::boundary_tree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->border_elements = new std::vector<std::unordered_map<uint64_t, boundary_node *>*>();
    for(auto b : TBordersVector){
        this->border_elements->push_back(new std::unordered_map<uint64_t, boundary_node *>());
    }
    this->boundary_tree_lroot = new std::unordered_map<uint64_t, boundary_node*>();
}

boundary_tree::boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
                             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->border_elements=border_elements;
    this->boundary_tree_lroot = new std::unordered_map<uint64_t, boundary_node*>();
}

boundary_tree::~boundary_tree(){
    for(uint32_t i=0;i < this->border_elements->size(); i++){
        for(auto pairs: *this->border_elements->at(i)){
            uint64_t n = pairs.first;
            delete pairs.second;
        }
        delete this->border_elements->at(i);
    }
    delete this->border_elements;
    delete this->boundary_tree_lroot;
}

void boundary_tree::insert_element(boundary_node &n, enum borders b, int64_t origin){
    boundary_node *new_n;
    auto border = this->border_elements->at(b);
    new_n = new boundary_node(n);
    border->emplace(n.maxtree_idx, new_n);
    if(this->boundary_tree_lroot->find(n.maxtree_idx) != this->boundary_tree_lroot->end()){
        this->boundary_tree_lroot->emplace(new_n->maxtree_idx, new_n);
    }
}

bool boundary_tree::insert_lroot(boundary_node *n){
    if(this->boundary_tree_lroot->find(n->maxtree_idx) != this->boundary_tree_lroot->end()){
        return false;
    }
    this->boundary_tree_lroot->emplace(n->maxtree_idx, n);
    return true;
}

void boundary_tree::add_lroot_tree(maxtree_node *levelroot, int64_t origin, std::vector<maxtree_node*> *maxtree_data){
    maxtree_node *parent;
    boundary_node *current, *bound_parent;
    int64_t parent_idx;
    int64_t pidx;
    
    if(this->boundary_tree_lroot->find(levelroot->idx) != this->boundary_tree_lroot->end()){ 
        // levelroot found on boundary tree, the branch is already added to boundary tree
        //current = this->get_border_node_lroot(levelroot->idx);
        return;
    }else{
        current = new boundary_node(levelroot, origin, -1);
        this->insert_lroot(current);
    }
    while(current != NULL){
        pidx = maxtree_data->at(current->maxtree_idx)->parent; // get parent idx of current boundary node
        if(this->boundary_tree_lroot->find(pidx) == this->boundary_tree_lroot->end()){// parent isn't in boundary tree
            if(pidx >= 0){// if this node has a parent (not the tile root)
                parent = maxtree_data->at(pidx);
                bound_parent = new boundary_node(parent, origin, -1); //create the parent node to add on bondary tree
                current->boundary_parent = bound_parent->maxtree_idx; //vinculate the idx (used in maxtree) of parent to the current node
                if(!this->insert_lroot(bound_parent)){ // try to insert parent node 
                    delete bound_parent; // if isn't possible insert node (the node is alredy in boundary tree) free its memory
                    break; // stop the insertion process (all the ancestors from current are in boundary tree)
                }
            }else{ // parent is in boundary tree, so we can stop the add process
                break;
            }
        }
        current = this->get_border_node_lroot(pidx); // go to the parent and add its ancerstors
    }
}

boundary_node *boundary_tree::get_border_node_lroot(int64_t maxtree_idx){
    boundary_node *ret;
    if (maxtree_idx >= 0){
        ret = this->boundary_tree_lroot->at(maxtree_idx);
    }else{
        ret = NULL;
    }
    return ret;
}

void boundary_tree::merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node){
    boundary_node *x = this->boundary_tree_lroot->at(this_node->maxtree_levelroot);
    boundary_node *y = t->boundary_tree_lroot->at(t_node->maxtree_levelroot);

    while(x->global_idx != y->global_idx){
        
    }
    
}

void boundary_tree::merge(boundary_tree *t, enum merge_directions d, uint8_t connection){
    if(connection != 4){
        std::cerr << "connection != 4 not implemented yet\n";
        exit(0);
    }
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

    std::cout << "local idx:\n";
    
    for(auto x: *v_this){
        std::cout << x.first <<" " ;
    }
    std::cout << "\n";
    
    for(auto x: *v_t){
        std::cout << x.first <<" ";
    }
    std::cout << "\nglobal idx\n";

    
    for(auto x: *v_this){
        std::cout << x.second->global_idx <<" " ;
    }
    std::cout << "\n";
    
    for(auto x: *v_t){
        std::cout << x.second->global_idx <<" ";
    }
    std::cout << "\n";
    
    
    if(v_t->size() != v_this->size()){
        std::cerr << "invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                  << ") and (" << t->grid_i  << ", " << t->grid_j << "\n";
        exit(0);
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
                //ss << bn->boundary_parent;
            }else if(f == BOUNDARY_LEVELROOT){
                //ss << bn->boundary_levelroot;
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


