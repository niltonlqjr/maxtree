#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin,
                             uint64_t global_idx, Tattribute a ,int64_t bound_parent){
    this->gval = gval;
    this->maxtree_idx = maxtree_idx;
    this->origin = origin;
    this->global_idx = global_idx;
    this->boundary_parent = bound_parent;
    this->attr = a;

}

boundary_node::boundary_node(maxtree_node *n, uint64_t origin,
                             int64_t bound_parent){
    this->gval = n->gval;
    this->maxtree_idx = n->idx;
    this->origin = origin;
    this->global_idx = n->global_idx;
    this->boundary_parent = bound_parent;
    this->attr = n->attribute;
    
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
    this->border_lr = -1;
}

boundary_tree::boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
                             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->border_elements=border_elements;
    this->boundary_tree_lroot = new std::unordered_map<uint64_t, boundary_node*>();
    this->border_lr = -1;
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
    border->emplace(n.global_idx, new_n);
    if(this->boundary_tree_lroot->find(n.global_idx) != this->boundary_tree_lroot->end()){
        this->boundary_tree_lroot->emplace(new_n->global_idx, new_n);
    }
}

bool boundary_tree::insert_lroot(boundary_node *n){
    if(this->boundary_tree_lroot->find(n->global_idx) != this->boundary_tree_lroot->end()){
        return false;
    }
    this->boundary_tree_lroot->emplace(n->global_idx, n);
    return true;
}

void boundary_tree::add_lroot_tree(maxtree_node *levelroot, int64_t origin, std::vector<maxtree_node*> *maxtree_data){
    maxtree_node *parent;
    boundary_node *current, *bound_parent;
    int64_t parent_idx;
    int64_t pidx;
    
    if(this->boundary_tree_lroot->find(levelroot->global_idx) != this->boundary_tree_lroot->end()){ 
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
                current->boundary_parent = bound_parent->global_idx; //vinculate the idx (used in maxtree) of parent to the current node
                if(!this->insert_lroot(bound_parent)){ // try to insert parent node 
                    delete bound_parent; // if isn't possible insert node (the node is alredy in boundary tree) free its memory
                    break; // stop the insertion process (all the ancestors from current are in boundary tree)
                }
            }else{ // parent is in boundary tree, so we can stop the add process
                break;
            }
        }
        current = this->get_border_node_lroot(parent->global_idx); // go to the parent and add its ancerstors
    }
}

boundary_node *boundary_tree::get_border_node_lroot(int64_t global_idx){
    boundary_node *ret;
    if (global_idx >= 0){
        return this->boundary_tree_lroot->at(global_idx);
    }
    return NULL;
}

bool boundary_tree::is_root(uint64_t n_idx){
    if(this->boundary_tree_lroot->find(n_idx) == this->boundary_tree_lroot->end()){
        return false;
    }
    return this->get_border_node_lroot(n_idx)->boundary_parent == -1;
}

void boundary_tree::merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node, boundary_tree *ret_tree){
    boundary_tree *x_tree = this;
    boundary_tree *y_tree = t;
    boundary_tree *z_tree = this;
    boundary_node *x = x_tree->boundary_tree_lroot->at(this_node->maxtree_levelroot);
    boundary_node *y = t->boundary_tree_lroot->at(t_node->maxtree_levelroot);
    boundary_node *z;

    while(x->global_idx != y->global_idx && !y_tree->is_root(y->maxtree_idx) ){
        z = z_tree->get_border_node_lroot(x->maxtree_idx);
        if(!z_tree->is_root(z->maxtree_idx) && z->gval >= y->gval){
            // merge attributes
            z = x;
            z_tree = x_tree;
        }else{
            if(x->gval == y->gval && x_tree != y_tree){
                if(y_tree->border_lr != NO_BORDER_LEVELROOT){// check if y_tree has border levelroot
                    x_tree->border_lr = y_tree->border_lr;
                    x->boundary_parent = x_tree->border_lr;
                }else if(x_tree->border_lr != NO_BORDER_LEVELROOT){ // check if 
                    y_tree->border_lr = x_tree->border_lr;
                    y->boundary_parent = y_tree->border_lr
                }else{
                    // cria o levelroot global desta área como sendo o par (x,y)
                    x->boundary_parent = y->global_idx;
                    //add y in x tree here
                }
            }else{
                x->boundary_parent = y->global_idx;
                //add y in x tree here
            }
            // manipulate atributes here
            x = y;
            x_tree = y_tree;
            y = z;
            y_tree = z_tree;
        }
    }
    if(y_tree->is_root(y->maxtree_idx)){
        while(!x_tree->is_root(x->maxtree_idx)){
            //merge data here
            z = x_tree->boundary_tree_lroot->at(x->maxtree_idx);
        }
    }
    
}

boundary_tree *boundary_tree::merge(boundary_tree *t, enum merge_directions d, uint8_t connection){
    if(connection != 4){
        std::cerr << "connection != 4 not implemented yet\n";
        exit(0);
    }
    uint64_t i ;
    std::unordered_map<uint64_t, boundary_node *> *v_this, *v_t, *aux_border;
    boundary_tree *ret_tree;

    if(d == MERGE_HORIZONTAL){    
        if(this->grid_i < t->grid_i){
            ret_tree = new boundary_tree(this->h + t->h - 2, this->w, this->grid_i, this->grid_j);
            v_this = this->border_elements->at(BOTTOM_BORDER);
            v_t = t->border_elements->at(TOP_BORDER);

            if(v_t->size() != v_this->size()){
                std::cerr << "invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                        << ") and (" << t->grid_i  << ", " << t->grid_j << "\n";
                exit(0);
            }            

            /* ============== copy this tree top border ============== */
            aux_border = this->border_elements->at(TOP_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),TOP_BORDER);
            }
            /* ============== copy t tree bottom border ============== */
            aux_border = t->border_elements->at(BOTTOM_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),BOTTOM_BORDER);
            }
            /* ============== concatenate this tree left border with t left border ============== */
            aux_border = this->border_elements->at(LEFT_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),LEFT_BORDER);
            }
            aux_border = t->border_elements->at(LEFT_BORDER);
            // the two last elements of this left border are the same of
            // the two first elements of t left border
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),LEFT_BORDER);
            }
            /* ============== concatenate this tree right border with t right border ============== */
            aux_border = this->border_elements->at(RIGHT_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)), RIGHT_BORDER);
            }
            aux_border = t->border_elements->at(RIGHT_BORDER);
            // the two last elements of this right border are the same of
            // the two first elements of t right border
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)), RIGHT_BORDER);
            }


        }else{
            std::cerr << "Invalid merge call: destiny tree is in the postion ("
                      << this->grid_i << ", " << this->grid_j << ") and the merged tree is in the position("
                      << t->grid_i << ", " << t->grid_j << "). The destiny tree must be at top left of merged tree.";
            exit(EX_DATAERR);
        }
    }else if(d == MERGE_VERTICAL){
        if(this->grid_j < t->grid_j){
            ret_tree = new boundary_tree(this->h, this->w + t->w - 2, this->grid_i, this->grid_j);
            v_this = this->border_elements->at(RIGHT_BORDER);
            v_t = t->border_elements->at(LEFT_BORDER);

            if(v_t->size() != v_this->size()){
                std::cerr << "invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                        << ") and (" << t->grid_i  << ", " << t->grid_j << "\n";
                exit(0);
            }

             /* ============== copy this tree left border ============== */
            aux_border = this->border_elements->at(LEFT_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),LEFT_BORDER);
            }
            /* ============== copy t tree right border ============== */
            aux_border = t->border_elements->at(RIGHT_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),RIGHT_BORDER);
            }
            /* ============== concatenate this tree top border with t top border ============== */
            aux_border = this->border_elements->at(TOP_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),TOP_BORDER);
            }
            aux_border = t->border_elements->at(TOP_BORDER);
            // the two last elements of this top border are the same of
            // the two first elements of t top border
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),TOP_BORDER);
            }
            /* ============== concatenate this tree bottom border with t bottom border ============== */
            aux_border = this->border_elements->at(BOTTOM_BORDER);
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)),BOTTOM_BORDER);
            }
            aux_border = t->border_elements->at(BOTTOM_BORDER);
            // the two last elements of this bottom border are the same of
            // the two first elements of t bottom border
            for(auto n : *aux_border){
                i = n.second->global_idx;
                ret_tree->insert_element(*(aux_border->at(i)), BOTTOM_BORDER);
            }

        }else{
            std::cerr << "Invalid merge call: destiny tree is in the postion ("
                      << this->grid_i << ", " << this->grid_j << ") and the merged tree is in the position("
                      << t->grid_i << ", " << t->grid_j << "). The destiny tree must be at top left of merged tree.";
            exit(EX_DATAERR);
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
    


    
    return ret_tree;


}

uint64_t boundary_tree::index_of(uint32_t l, uint32_t c){
    return l * this->w + c;
}

std::tuple<uint32_t,uint32_t> boundary_tree::lin_col(uint64_t index){
    return std::make_tuple(index / this->w, index % this->w);
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
            }else if(f==BOUNDARY_GLOBAL_IDX){
                ss << bn->global_idx;
            }
            ss << " ";
        }
        ss << "\n";
    }
    return ss.str();
}


