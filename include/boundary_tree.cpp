#include "boundary_tree.hpp"


boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin,
                             uint64_t global_idx, Tattribute a ,int64_t bound_parent,
                             int64_t border_lr){
    this->gval = gval;
    this->maxtree_idx = maxtree_idx;
    this->origin = origin;
    this->global_idx = global_idx;
    this->boundary_parent = bound_parent;
    this->attr = a;
    this->border_lr = border_lr;

}

boundary_node::boundary_node(maxtree_node *n, uint64_t origin,
                             int64_t bound_parent, int64_t border_lr){
    this->gval = n->gval;
    this->maxtree_idx = n->idx;
    this->origin = origin;
    this->global_idx = n->global_idx;
    this->boundary_parent = bound_parent;
    this->attr = n->attribute;
    this->border_lr = border_lr;
    
}

boundary_tree::boundary_tree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    //this->border_elements = new std::vector<std::unordered_map<uint64_t, boundary_node *>*>();
    this->border_elements = new std::vector<std::vector<boundary_node *>*>();
    for(auto b : TBordersVector){
        //this->border_elements->push_back(new std::unordered_map<uint64_t, boundary_node *>());
        this->border_elements->push_back(new std::vector<boundary_node *>());
    }
    this->boundary_tree_lroot = new std::unordered_map<uint64_t, boundary_node*>();
    //this->border_lr = -1;
}
/*
boundary_tree::boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
                             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->border_elements=border_elements;
    this->boundary_tree_lroot = new std::unordered_map<uint64_t, boundary_node*>();
    this->border_lr = -1;
}*/

boundary_tree::~boundary_tree(){
/*    for(uint32_t i=0;i < this->border_elements->size(); i++){
        for(auto pairs: *this->border_elements->at(i)){
            uint64_t n = pairs.first;
            delete pairs.second;
        }
        delete this->border_elements->at(i);
    }
    delete this->border_elements;
    delete this->boundary_tree_lroot;*/
}

void boundary_tree::insert_border_element(boundary_node &n, enum borders b, int64_t origin){
    boundary_node *new_n;
    auto border = this->border_elements->at(b);
    new_n = new boundary_node(n);
    //border->emplace(n.maxtree_idx, new_n);
    //border->emplace(border->size(), new_n);
    border->push_back(new_n);
    /* if(this->boundary_tree_lroot->find(n.maxtree_idx) != this->boundary_tree_lroot->end()){
        this->boundary_tree_lroot->emplace(new_n->maxtree_idx, new_n);
        //this->boundary_tree_lroot->emplace(border->size(), new_n);
    } */
}

bool boundary_tree::insert_bnode_lroot_tree(boundary_node *n){
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
    int64_t global_pidx, local_pidx, pidx;
    
    if(this->boundary_tree_lroot->find(levelroot->global_idx) != this->boundary_tree_lroot->end()){ 
        // levelroot found on boundary tree, the branch is already added to boundary tree
        //current = this->get_border_node_lroot(levelroot->idx);

        return;
    }else{
        current = new boundary_node(levelroot, origin, -1);
        this->insert_bnode_lroot_tree(current);
    }
    while(current != NULL){
        pidx = maxtree_data->at(current->maxtree_idx)->parent; // get parent idx of current boundary node
        if(pidx < 0){
            break;
        }
        /* global_pidx = maxtree_data->at(local_pidx)->idx;
        if(global_pidx < 0){
            break;
        } */
        parent = maxtree_data->at(pidx);
        if(this->boundary_tree_lroot->find(parent->global_idx) == this->boundary_tree_lroot->end()){// parent isn't in boundary tree
            bound_parent = new boundary_node(parent, origin, -1); //create the parent node to add on bondary tree
            current->boundary_parent = bound_parent->global_idx; //vinculate the idx (used in maxtree) of parent to the current node
            if(!this->insert_bnode_lroot_tree(bound_parent)){ // try to insert parent node 
                delete bound_parent; // if isn't possible insert node (the parent node is alredy in boundary tree) free memory of last 
                break; // stop the insertion process (all the ancestors from current are in boundary tree)
            }
        }else{ // parent inode s in boundary tree, so it doesn't need to be inserted, just update parent relation and stop process
            current->boundary_parent = parent->global_idx;
            break;
        }
        current = this->get_border_node_lroot(parent->global_idx); // go to the parent and add its ancerstors
    }
}

boundary_node *boundary_tree::get_border_node_lroot(int64_t global_idx){
    if(this->boundary_tree_lroot->find(global_idx) != this->boundary_tree_lroot->end())
        return this->boundary_tree_lroot->at(global_idx);
    return NULL;
}

uint64_t boundary_tree::get_lroot_tree_size(){
    return this->boundary_tree_lroot->size();
}

uint64_t boundary_tree::get_border_size(){
    uint64_t s = 0;
    for(enum borders b: TBordersVector){
        s += this->border_elements->at(b)->size();
    }
    return s;
}

void boundary_tree::change_border(std::vector<boundary_node *> *new_border, enum borders b){
    delete this->border_elements;
    this->border_elements->at(b) = new_border;
}

bool boundary_tree::is_root(uint64_t n_idx){
    if(this->boundary_tree_lroot->find(n_idx) == this->boundary_tree_lroot->end()){
        return false;
    }
    int64_t bound_par_idx = this->get_border_node_lroot(n_idx)->boundary_parent;
    if(bound_par_idx != -1){
        return false;
    }
    return true;
}

void boundary_tree::merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node, boundary_tree *ret_tree){
    boundary_tree *x_tree = this;
    boundary_tree *y_tree = t;
    boundary_tree *z_tree = this;
    boundary_node *x = x_tree->get_border_node_lroot(this_node->global_idx);
    boundary_node *y = t->get_border_node_lroot(t_node->global_idx);
    boundary_node *z;
    
    while(x->global_idx != y->global_idx){//&& !y_tree->is_root(y->global_idx) ){
        z = z_tree->get_border_node_lroot(x->boundary_parent);
        std::cout << "z:" << z->global_idx << " x:" << x->global_idx << " y:" << y->global_idx << "\n";
        if(!z_tree->is_root(z->global_idx) && z->gval >= y->gval){
            // merge attributes
            x = z;
            x_tree = z_tree;
        }else{
            if(x->gval == y->gval && x_tree != y_tree){
                if(y->border_lr != NO_BORDER_LEVELROOT){// check if y_tree has border levelroot
                    x->border_lr = y->border_lr;
                    x->boundary_parent = y->border_lr;
                }else if(x->border_lr != NO_BORDER_LEVELROOT){ // check if x_tree has border levelroot
                    y->border_lr = x->border_lr;
                    y->boundary_parent = x->border_lr;
                }else{
                    // cria o levelroot global desta área como sendo o par (x,y)
                    x->border_lr = y->global_idx;
                    y->border_lr = y->global_idx;
                    x->boundary_parent = y->global_idx;
                    x_tree->insert_bnode_lroot_tree(y);
                    //add y in x tree here
                }
            }else{
                x->boundary_parent = y->global_idx;
                x_tree->insert_bnode_lroot_tree(y);
                //add y in x tree here
            }
            // manipulate atributes here
            x = y;
            x_tree = y_tree;
            y = z;
            y_tree = z_tree;
        }
    }
    if(y_tree->is_root(y->global_idx)){
        while(!x_tree->is_root(x->global_idx)){
            //merge data here
            std::cout << "z:" << z->global_idx << " x:" << x->global_idx << " y:" << y->global_idx << "\n";

            x = x_tree->boundary_tree_lroot->at(x->boundary_parent);
        }
    }
}


boundary_tree *boundary_tree::merge(boundary_tree *t, enum merge_directions d, uint8_t connection, bool verbose){
    if(connection != 4){
        std::cerr << "connection != 4 not implemented yet\n";
        exit(EX_DATAERR);
    }
    uint64_t i ;
    //std::unordered_map<uint64_t, boundary_node *> *v_this, *v_t, *aux_border;
    std::vector< boundary_node *> *v_this, *v_t, *aux_border;
    boundary_tree *ret_tree;

    if(d == MERGE_HORIZONTAL){    
        if(this->grid_i < t->grid_i){
            ret_tree = new boundary_tree(this->h + t->h - 2, this->w, this->grid_i, this->grid_j);
            v_this = this->border_elements->at(BOTTOM_BORDER);
            v_t = t->border_elements->at(TOP_BORDER);

            if(v_t->size() != v_this->size()){
                std::cerr << "invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                          << ") and (" << t->grid_i  << ", " << t->grid_j << "\n";
                exit(EX_DATAERR);
            }            

            /* ============== copy this tree top border ============== */
            aux_border = this->border_elements->at(TOP_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),TOP_BORDER);
                ret_tree->insert_border_element(*n,TOP_BORDER);
            }
            /* ============== copy t tree bottom border ============== */
            aux_border = t->border_elements->at(BOTTOM_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),BOTTOM_BORDER);
                ret_tree->insert_border_element(*n,BOTTOM_BORDER);
            }
            /* ============== concatenate this tree left border with t left border ============== */
            aux_border = this->border_elements->at(LEFT_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),LEFT_BORDER);
                ret_tree->insert_border_element(*n,LEFT_BORDER);
            }
            aux_border = t->border_elements->at(LEFT_BORDER);
            // the two last elements of this left border are the same of
            // the two first elements of t left border
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),LEFT_BORDER);
                ret_tree->insert_border_element(*n,LEFT_BORDER);
            }
            /* ============== concatenate this tree right border with t right border ============== */
            aux_border = this->border_elements->at(RIGHT_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)), RIGHT_BORDER);
                ret_tree->insert_border_element(*n,RIGHT_BORDER);
            }
            aux_border = t->border_elements->at(RIGHT_BORDER);
            // the two last elements of this right border are the same of
            // the two first elements of t right border
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)), RIGHT_BORDER);
                ret_tree->insert_border_element(*n,RIGHT_BORDER);
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
                std::cerr << "Invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                        << ") and (" << t->grid_i  << ", " << t->grid_j << "\n";
                exit(EX_DATAERR);
            }

             /* ============== copy this tree left border ============== */
            aux_border = this->border_elements->at(LEFT_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),LEFT_BORDER);
                ret_tree->insert_border_element(*n,LEFT_BORDER);
            }
            /* ============== copy t tree right border ============== */
            aux_border = t->border_elements->at(RIGHT_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),RIGHT_BORDER);
                ret_tree->insert_border_element(*n,RIGHT_BORDER);
            }
            /* ============== concatenate this tree top border with t top border ============== */
            aux_border = this->border_elements->at(TOP_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),TOP_BORDER);
                ret_tree->insert_border_element(*n,TOP_BORDER);
            }
            aux_border = t->border_elements->at(TOP_BORDER);
            // the two last elements of this top border are the same of
            // the two first elements of t top border
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),TOP_BORDER);
                ret_tree->insert_border_element(*n,TOP_BORDER);
            }
            /* ============== concatenate this tree bottom border with t bottom border ============== */
            aux_border = this->border_elements->at(BOTTOM_BORDER);
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)),BOTTOM_BORDER);
                ret_tree->insert_border_element(*n,BOTTOM_BORDER);
            }
            aux_border = t->border_elements->at(BOTTOM_BORDER);
            // the two last elements of this bottom border are the same of
            // the two first elements of t bottom border
            for(auto n : *aux_border){
                //i = n.second->maxtree_idx;
                //ret_tree->insert_border_element(*(aux_border->at(i)), BOTTOM_BORDER);
                ret_tree->insert_border_element(*n,BOTTOM_BORDER);
            }

        }else{
            std::cerr << "Invalid merge call: destiny tree is in the postion ("
                      << this->grid_i << ", " << this->grid_j << ") and the merged tree is in the position("
                      << t->grid_i << ", " << t->grid_j << "). The destiny tree must be at top left of merged tree.";
            exit(EX_DATAERR);
        }
    }
    
    if(verbose){
        std::cout << "local idx:\n";
        
        for(uint64_t i = 0; i < v_this->size(); i++){
            boundary_node *x = v_this->at(i);
            std::cout << x->maxtree_idx <<" " ;
        }
        std::cout << "\n";
        
        for(uint64_t i = 0; i < v_t->size(); i++){
            boundary_node *x = v_t->at(i);
            std::cout << x->maxtree_idx <<" ";
        }
        std::cout << "\nglobal idx\n";

        
        for(uint64_t i = 0; i < v_this->size(); i++){
            boundary_node *x = v_this->at(i);
            std::cout << x->global_idx <<" " ;
        }
        std::cout << "\n";
        
       for(uint64_t i = 0; i < v_t->size(); i++){
            boundary_node *x = v_t->at(i);
            std::cout << x->global_idx <<" ";
        }
        std::cout << "\n";
        std::cout << "this tree:\n"
                  << this->lroot_to_string() << "\n"
                  << "to merge tree:\n"
                  << t->lroot_to_string() << "\n";
        std::cout << "=========================\n";
    }

    for(uint32_t i=0; i<v_this->size(); i++){
        boundary_node *x = this->get_border_node_lroot(v_this->at(i)->boundary_parent);
        if(x == NULL){
            x = this->get_border_node_lroot(v_this->at(i)->global_idx);
        }

        boundary_node *y = t->get_border_node_lroot(v_t->at(i)->boundary_parent);
        if(y == NULL){
            y = t->get_border_node_lroot(v_t->at(i)->global_idx);
        }

        if(verbose){
            std::cout << "merging: " << v_this->at(i)->global_idx << " (lroot:" << x->global_idx << ")\n"
                      << "   with: " << v_t->at(i)->global_idx << " (lroot:" << v_t->at(i)->boundary_parent << ")\n";
        }
        this->merge_branches(x,t,y,ret_tree);
        std::cout << "this new tree:" << this->lroot_to_string() << "\n";
        std::cout << "t new tree:" << t->lroot_to_string() << "\n";
    }
    
    return ret_tree;
}

uint64_t boundary_tree::index_of(uint32_t l, uint32_t c){
    return l * this->w + c;
}

std::tuple<uint32_t,uint32_t> boundary_tree::lin_col(uint64_t index){
    return std::make_tuple(index / this->w, index % this->w);
}

std::string boundary_tree::lroot_to_string(enum boundary_tree_field f){
    std::ostringstream ss;
    for(auto bn: *(this->boundary_tree_lroot)){
        if(f == BOUNDARY_PARENT){
            ss << "(" << bn.first << "," << bn.second->boundary_parent << ")";
/*         }else if(f == BOUNDARY_LEVELROOT){
            //ss << bn->boundary_levelroot;
        }else if(f == BOUNDARY_IDX){
            //ss << bn->boundary_idx; */
        }else if(f == MAXTREE_IDX){
            ss << "(" << bn.first << "," << bn.second->maxtree_idx << ")";
        }else if(f == BOUNDARY_GVAL){
            ss << "(" << bn.first << "," << bn.second->gval << ")";
        }else if(f==BOUNDARY_GLOBAL_IDX){
            ss << "(" << bn.first << "," << bn.second->global_idx << ")";
        }
        ss << " ";
    }
    return ss.str();
}

std::string boundary_tree::border_to_string(enum boundary_tree_field f){
    std::ostringstream ss;
    for(int i=0; i<NamesBordersVector.size();i++){
        ss << NamesBordersVector[i] << ":";
        auto v = *(this->border_elements->at(i));
        /*for(auto pairs: v){
            auto bn = pairs.second;*/
        for(auto bn: v){
            if(f == BOUNDARY_PARENT){
                ss << bn->boundary_parent;
/*             }else if(f == BOUNDARY_LEVELROOT){
                //ss << bn->boundary_levelroot;
            }else if(f == BOUNDARY_IDX){
                //ss << bn->boundary_idx; */
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


