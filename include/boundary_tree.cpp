#include "boundary_tree.hpp"


/* boundary_node::boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin, 
                             uint64_t global_idx, Tattribute a , int64_t bound_parent, 
                             int64_t border_lr){
    this->gval = gval;
    this->maxtree_idx = maxtree_idx;
    this->origin = origin;
    this->global_idx = global_idx;
    this->boundary_parent = bound_parent;
    this->attr = a;
    this->border_lr = border_lr;

}
 */
boundary_node::boundary_node(maxtree_node *n, boundary_tree *bound_tree_ptr, 
                             int64_t bound_parent, int64_t border_lr){
    // this->gval = n->gval;
    // this->maxtree_idx = n->idx;
    this->origin = origin;
    this->bound_tree_ptr = bound_tree_ptr;
    this->boundary_parent = bound_parent;
    // this->attr = n->attribute;
    this->border_lr = border_lr;
    this->ptr_node = n;
    
}

/* void boundary_node::accumulate_attr(boundary_node *merged){
    this->ptr_node->compute_attribute(merged->ptr_node->attribute);
} */
/* 
void boundary_node::accumulate_attr(Tattribute value){
    this->ptr_node->attribute += value;
}
 */

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
    this->tile_borders = new std::vector<bool>();
    for(int i=0; i<4; i++){
        this->tile_borders->push_back(false);
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
    for(uint32_t i=0;i < this->border_elements->size(); i++){
        for(auto e: *this->border_elements->at(i)){
            delete e;
        }
        delete this->border_elements->at(i);
    }
    delete this->border_elements;
    for(auto n: *(this->boundary_tree_lroot)){
        delete n.second;
    }
    delete this->boundary_tree_lroot;
}

boundary_node *boundary_tree::insert_border_element(boundary_node &n, enum borders b){
    boundary_node *new_n;
    auto border = this->border_elements->at(b);
    this->tile_borders->at(b) = true;
    new_n = new boundary_node(n);
    new_n->bound_tree_ptr = this;
    border->push_back(new_n);
    return new_n;
}

bool boundary_tree::insert_bnode_lroot_tree(boundary_node *n, bool copy){
    //if(this->boundary_tree_lroot->find(n->ptr_node->global_idx) != this->boundary_tree_lroot->end()){
    if(this->get_border_node_lroot(n->ptr_node->global_idx) != NULL){
        if(verbose) std::cout << "fail to inset: " << n->ptr_node->global_idx << " at tree\n";
        return false;
    }
    if(copy){
        n = new boundary_node(*n);
        n->bound_tree_ptr = this;
    }
    this->boundary_tree_lroot->emplace(n->ptr_node->global_idx, n);
    if(verbose) std::cout << "node " << n->ptr_node->global_idx << " sucessfully inserted \n";
    return true;
}

bool boundary_tree::remove_bnode_lroot_tree(int64_t global_idx){
    if(this->boundary_tree_lroot->find(global_idx) != this->boundary_tree_lroot->end()){
        this->boundary_tree_lroot->erase(global_idx);
        return true;
    }
    return false;
}

void boundary_tree::add_lroot_tree(boundary_node *levelroot, bool insert_ancestors,
                                   bool copy){
    bool inserted=true;
    char __CH;
    boundary_node *parent, *current, *lroot_at_this, *parent_at_this;
    int64_t pidx;
    boundary_tree *t_levelroot;


    t_levelroot = levelroot->bound_tree_ptr; 
    lroot_at_this=this->get_border_node_lroot(levelroot->ptr_node->global_idx);
    if(verbose){
        std::cout << "add levelroot by boundary node:" << levelroot->ptr_node->global_idx << "\n";
        if(lroot_at_this){
            std::cout << "lroot_at_this:" << lroot_at_this->ptr_node->global_idx << "\n";
        }else{
            std::cout << "lroot_at_this: NULL\n";
        }
    }
    if(lroot_at_this == NULL){
        this->insert_bnode_lroot_tree(levelroot,copy);
    }

    if(insert_ancestors){
        current = levelroot;
        while(current != NULL){
            if(verbose) std::cout << " This tree: "<< this->lroot_to_string() << "\n";
            if(verbose) std::cout << " levelroot tree: "<< t_levelroot->lroot_to_string() << "\n";
            pidx = current->boundary_parent;
            
            if(pidx < 0){
                break;
            }
            parent = t_levelroot->get_border_node_lroot(pidx);

            current = parent;
            
            if(!parent){
                break;
            }

            if(!parent || !current){
                std:: cout << "current: " << current->ptr_node->global_idx << " parent: " << parent->ptr_node->global_idx << "\n";
            }

            if(parent!=NULL){

                if(!copy){
                    inserted = this->insert_bnode_lroot_tree(parent);
                }else{
                    auto new_lr = new boundary_node(parent->ptr_node, this, parent->boundary_parent, parent->border_lr);
                    if(verbose) std::cout << new_lr->ptr_node->global_idx << "\n";
                    inserted = this->insert_bnode_lroot_tree(new_lr);
                    if(!inserted){
                        delete new_lr;
                    }
                }
            }
            
        }
    }
}

void boundary_tree::add_lroot_tree(maxtree_node *levelroot, std::vector<maxtree_node*> *maxtree_data, bool insert_ancestors){
    maxtree_node *parent;
    boundary_node *current, *bound_parent;
    int64_t pidx;
    
    //if(this->boundary_tree_lroot->find(levelroot->ptr_node->global_idx) != this->boundary_tree_lroot->end()){ 
    if(this->get_border_node_lroot(levelroot->global_idx) != NULL){ 
        // levelroot found on boundary tree, the branch is already added to boundary tree
        //current = this->get_border_node_lroot(levelroot->idx);
        return;
    }else{
        current = new boundary_node(levelroot, this, -1);
        this->insert_bnode_lroot_tree(current);
    }

    if(insert_ancestors){
        while(current != NULL){
            pidx = maxtree_data->at(current->ptr_node->idx)->parent; // get parent idx of current boundary node
            if(pidx < 0){
                break;
            }
            /* global_pidx = maxtree_data->at(local_pidx)->idx;
            if(global_pidx < 0){
                break;
            } */
            parent = maxtree_data->at(pidx);
            if(this->boundary_tree_lroot->find(parent->global_idx) == this->boundary_tree_lroot->end()){// parent isn't in boundary tree
                bound_parent = new boundary_node(parent, this, -1); //create the parent node to add on bondary tree
                current->boundary_parent = bound_parent->ptr_node->global_idx; //vinculate the idx (used in maxtree) of parent to the current node
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
}

boundary_node *boundary_tree::get_border_node_lroot(int64_t global_idx){
    if(this->boundary_tree_lroot->find(global_idx) != this->boundary_tree_lroot->end())
        return this->boundary_tree_lroot->at(global_idx);
    return NULL;
}
std::vector<boundary_node *> *boundary_tree::get_border(enum borders b){
    return this->border_elements->at(b);
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
    delete this->border_elements->at(b);
    this->border_elements->at(b) = new_border;
    if(new_border->size() <= 0){
        this->tile_borders->at(b) = false;
    }else{
        this->tile_borders->at(b) = true;
    }
    if(b == LEFT_BORDER || b == RIGHT_BORDER){
        this->h = new_border->size();
    }else if(b == TOP_BORDER || b == BOTTOM_BORDER){
        this->w = new_border->size();
    }
}

bool boundary_tree::is_root(uint64_t n_idx){
    if(this->get_border_node_lroot(n_idx) == NULL){
        return false;
    }
    int64_t bound_par_idx = this->get_border_node_lroot(n_idx)->boundary_parent;
    if(bound_par_idx != -1){
        return false;
    }
    return true;
}

// void boundary_tree::merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node){
void boundary_tree::merge_branches(boundary_node *this_node, boundary_node *t_node, 
                                   std::unordered_map<uint64_t, bool> &acc){//}, std::vector<u_int64_t> &swap_nodes){
    boundary_tree *t = t_node->bound_tree_ptr;
    boundary_node *x, *y, *z, *h;
    Tattribute attrx, attry, attr_aux, attr_aux1;
    uint64_t xidx;
    attr_aux  = attr_aux1 = Tattr_NULL;
    x = this_node->bound_tree_ptr->get_border_node_lroot(this_node->ptr_node->global_idx);
    y = t_node->bound_tree_ptr->get_border_node_lroot(t_node->ptr_node->global_idx);



    if(verbose){
        std::cout << "______________________________________\n";
        std::cout << "merging branches: " << this_node->ptr_node->global_idx 
                  << " and " << t_node->ptr_node->global_idx << "\n";
    }

    //Fuse (the merge branches algorithm) from kazemier work

    if(x->ptr_node->gval < y->ptr_node->gval){
        auto aux = x;
        x = y;
        y = aux;
    }
    //while(x->ptr_node->global_idx != y->ptr_node->global_idx && !y->bound_tree_ptr->is_root(y->ptr_node->idx)){
    if(verbose)
        std::cout << "\tthis_node=" << this_node->ptr_node->global_idx << "\n";
    while(y!=NULL && x->ptr_node->global_idx != y->ptr_node->global_idx){
        z = x->bound_tree_ptr->get_border_node_lroot(x->boundary_parent);
        if(verbose){
            std::cout << "attr aux:" << attr_aux << "\n";
            std::cout <<" merge branches elements: x=" << x->ptr_node->global_idx 
                      << " y=" << y->ptr_node->global_idx << " z=" << (z==NULL ? (int64_t) -1 : (int64_t) z->ptr_node->global_idx)
                      << "\n";
        }
        auto xidx = x->ptr_node->global_idx;
        //if(!z->bound_tree_ptr->is_root(z->ptr_node->global_idx) && z->ptr_node->gval >= y->ptr_node->gval){
        if(z != NULL && z->ptr_node->gval >= y->ptr_node->gval){
            if (verbose) 
                std::cout << "y:" << y->ptr_node->global_idx << " z:" << z->ptr_node->global_idx << " x:" << xidx <<"\n";
            if(acc.find(xidx) == acc.end() || acc[xidx] == false){
                x->ptr_node->attribute += attr_aux;
                if(verbose){
                    std::cout << "adding " << attr_aux << " on node " << x->ptr_node->global_idx << " x->attr = " << x->ptr_node->attribute
                              << " line:" << __LINE__ << " <------------ \n";
                }
                if(this->get_border_node_lroot(x->ptr_node->global_idx) == NULL){
                    if(verbose){
                        std::cout << "=====> adding " << x->ptr_node->global_idx << " to this tree\n" << "x levrooot "
                                  << x->bound_tree_ptr->get_border_node_lroot(xidx)->ptr_node->global_idx
                                  << "\n";
                    }
                    //this->add_lroot_tree(x,true,true);
                    acc[xidx] = true;
                }
            }
            x = z;
        }else{
            if(acc.find(xidx) == acc.end() || acc[xidx] == false){
                if(x->bound_tree_ptr != this){
                    this->add_lroot_tree(x,true,true);
                }
                this->get_border_node_lroot(xidx)->border_lr = y->ptr_node->global_idx;
                x->bound_tree_ptr->add_lroot_tree(y,true,true);
                
                if(verbose){
                    std::cout << x->ptr_node->global_idx << "->border_lr = " << y->ptr_node->global_idx << "\n";
                    std::cout << "::::::> adding " << y->ptr_node->global_idx << " to x tree\n";
                }
                if(this->get_border_node_lroot(x->ptr_node->global_idx) == NULL){
                    if(verbose)
                    std::cout << "---->adding " << x->ptr_node->global_idx << " to this tree\n";
                    this->add_lroot_tree(x,true,true);   
                }
                attr_aux1 = attr_aux + x->ptr_node->attribute;
                if(verbose){
                    std::cout << "adding " << attr_aux << " on node " << x->ptr_node->global_idx << " x->attr = " << attr_aux1
                              << " line:" << __LINE__ << " <------------ \n";
                    std::cout << "in this case, y=" << y->ptr_node->global_idx << "\n";
                }
                attr_aux = x->ptr_node->attribute;
                x->ptr_node->attribute = attr_aux1;
                acc[xidx] = true;
            }
            x = y;
            y = z;
        }
    }
    //if(y->bound_tree_ptr->is_root(y->ptr_node->global_idx)){
    if(y==NULL){
        //while(!x->bound_tree_ptr->is_root(x->ptr_node->global_idx)){
        while(x!=NULL){
            //x->accumulate_attr(attr_aux);
            if(acc.find(x->ptr_node->global_idx) == acc.end() || acc[xidx] == false){
                x->ptr_node->attribute += attr_aux;
                acc[x->ptr_node->global_idx] = true;
                if(verbose){
                    std::cout << "adding " << attr_aux << " on node " << x->ptr_node->global_idx << " x->attr = " << x->ptr_node->attribute
                              << " line:" << __LINE__ << " <------------ \n";
                }
            }
            x = x->bound_tree_ptr->get_border_node_lroot(x->boundary_parent);
        }
    }
    //std::cout << this->lroot_to_string() << "\n";
    /* 
    if(verbose){
        if(y) std::cout << "y_tree:" << y->bound_tree_ptr->lroot_to_string() << "\n";
        if(x) std::cout << "x_tree:" << x->bound_tree_ptr->lroot_to_string() << "\n";
        if(z) std::cout << "z_tree:" << z->bound_tree_ptr->lroot_to_string() << "\n";  
        if(this_node) std::cout << "this_node tree" << this_node->bound_tree_ptr->lroot_to_string(BOUNDARY_PARENT) << "\n";
        if(t_node) std::cout << "t_node tree" << t_node->bound_tree_ptr->lroot_to_string(BOUNDARY_PARENT) << "\n";
        //getchar();
        std::cout << "______________________________________\n";
        if(this_node) std::cout << "end merge: " << this_node->ptr_node->global_idx << " and " << t_node->ptr_node->global_idx << "\n";
    } 
 */
}

void boundary_tree::combine_borders(boundary_tree *t1, boundary_tree *t2, enum merge_directions d){
    std::vector<boundary_node *> *v_t1, *v_t2, *new_border;
    enum borders first_border, second_border, third_border, fourth_border;
    uint64_t ini ;

    if(d == MERGE_VERTICAL){
        first_border=LEFT_BORDER; second_border=RIGHT_BORDER; 
        third_border=TOP_BORDER; fourth_border=BOTTOM_BORDER;
    }else if(d == MERGE_HORIZONTAL){
        first_border=TOP_BORDER; second_border=BOTTOM_BORDER; 
        third_border=LEFT_BORDER; fourth_border=RIGHT_BORDER;
    }
    bool has_border[] = {false, false, false, false};
    //first_border comes from t1, second_border comes from t2, third_border (and fourth) are merged first half from t1, second half from t2
    
    /*copy first border from t1*/
    new_border = new std::vector<boundary_node *>();
    v_t1 = t1->get_border(first_border);
    
    for(uint64_t i = 0; i < v_t1->size(); i++){
        new_border->push_back(v_t1->at(i));
    }
    this->change_border(new_border, first_border);

    /*copy second border from t2*/
    new_border = new std::vector<boundary_node *>();
    v_t2 = t2->get_border(second_border);
    
    for(uint64_t i = 0; i < v_t2->size(); i++){
        new_border->push_back(v_t2->at(i));
    }
    this->change_border(new_border, second_border);
    
    /*merge third border, first half from t1, second from t2*/
    new_border = new std::vector<boundary_node *>();
    v_t1 = t1->get_border(third_border);
    v_t2 = t2->get_border(third_border);
    
    for(uint64_t i = 0; i < v_t1->size(); i++){
        new_border->push_back(v_t1->at(i));
    }

    ini = 0 + t2->tile_borders->at(first_border) + t1->tile_borders->at(second_border);
    for(uint64_t i = ini; i < v_t2->size(); i++){
        new_border->push_back(v_t2->at(i));
    }
    this->change_border(new_border, third_border);
    
    new_border = new std::vector<boundary_node *>();
    v_t1 = t1->border_elements->at(fourth_border);
    v_t2 = t2->border_elements->at(fourth_border);

    for(uint64_t i = 0; i < v_t1->size(); i++){
        new_border->push_back(v_t1->at(i));
    }

    ini = 0 + t2->tile_borders->at(first_border) + t1->tile_borders->at(second_border);

    for(uint64_t i = ini; i < v_t2->size(); i++){
        new_border->push_back(v_t2->at(i));
    }
    this->change_border(new_border, fourth_border);
    
}

boundary_tree *boundary_tree::get_copy(){
    boundary_tree *copy;
    copy = new boundary_tree(this->h, this->w, this->grid_i, this->grid_j);
    /* copy all borders and insert its subtrees*/
    for(borders b: TBordersVector){
        for(boundary_node *n: *(this->border_elements->at(b))){
            copy->insert_border_element(*n, b);
        }
    }
    
    for(auto pair: *(this->boundary_tree_lroot)){
        boundary_node *n = new boundary_node(*(pair.second));
        if(!copy->insert_bnode_lroot_tree(n,true)){
            delete n;
        }
    }
    return copy;
}

boundary_tree *boundary_tree::merge(boundary_tree *t, enum merge_directions d, uint8_t connection){
    if(connection != 4){
        std::cerr << "connection != 4 not implemented yet\n";
        exit(EX_DATAERR);
    }
    uint64_t i;
    //std::unordered_map<uint64_t, boundary_node *> *v_this, *v_t, *aux_border;
    std::vector< boundary_node *> *v_this, *v_t, *aux_border, *v_ret;
    boundary_tree *ret_tree, *merge_tree;
    std::unordered_map<uint64_t, bool> accumulated;
    std::vector<u_int64_t> swap_nodes;
    /*given that the levelroots will be visited a lot of times, it is needed to sum only once,
     so this map keeps the global ids already summed in this merge operation*/


    ret_tree = this->get_copy();
    merge_tree = t->get_copy();
    if(d == MERGE_HORIZONTAL){  // prepare data to merge borders placed on horizontal (this tree bottom border and merged tree top border)
        if(this->grid_i < merge_tree->grid_i){
            
            v_this = this->border_elements->at(BOTTOM_BORDER);
            v_ret = ret_tree->border_elements->at(BOTTOM_BORDER);
            v_t = merge_tree->border_elements->at(TOP_BORDER);

            if(v_t->size() != v_this->size()){
                std::cerr << "invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                          << ") and (" << merge_tree->grid_i  << ", " << merge_tree->grid_j << "\n";
                exit(EX_DATAERR);
            }     
        }else{
            std::cerr << "Invalid merge call: destiny tree is in the postion ("
                      << this->grid_i << ", " << this->grid_j << ") and the merged tree is in the position("
                      << merge_tree->grid_i << ", " << merge_tree->grid_j << "). The destiny tree must be at top left of merged tree.\n";
            exit(EX_DATAERR);
        }
    }else if(d == MERGE_VERTICAL){ // prepare data to merger borders placed on vertical (this tree right border and merged tree left border)
        if(this->grid_j < merge_tree->grid_j){
            
            v_this = this->border_elements->at(RIGHT_BORDER);
            v_ret = ret_tree->border_elements->at(RIGHT_BORDER);
            v_t = merge_tree->border_elements->at(LEFT_BORDER);

            if(v_t->size() != v_this->size()){
                std::cerr << "Invalid borders for tiles (" << this->grid_i << ", " << this->grid_j
                        << ") and (" << merge_tree->grid_i  << ", " << merge_tree->grid_j << "\n";
                exit(EX_DATAERR);
            }
        }else{
            std::cerr   << "Invalid merge call: destiny tree is in the postion ("
                        << this->grid_i << ", " << this->grid_j
                        << ") and the merged tree is in the position("
                        << merge_tree->grid_i << ", " << merge_tree->grid_j 
                        << "). The destiny tree must be at top left of merged tree.\n";
            exit(EX_DATAERR);
        }
    }
    

    if(verbose){
        std::cout << "merge new tree:\n";//" << t->lroot_to_string() << "\n";
        merge_tree->print_tree();
        std::cout << "Atributes:\n";
        std::cout << merge_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
        std::cout << "____________________________________________________________\n";

        std::cout << "ret_tree new tree:\n";// << ret_tree->lroot_to_string() << "\n";
        ret_tree->print_tree();
        std::cout << "Atributes:\n";
        std::cout << ret_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
        std::cout << "____________________________________________________________\n";
    }


    for(uint32_t i=0; i<v_ret->size(); i++){
        boundary_node *x = ret_tree->get_border_node_lroot(v_ret->at(i)->boundary_parent);
        if(x == NULL){
            x = ret_tree->get_border_node_lroot(v_ret->at(i)->ptr_node->global_idx);
        }

        boundary_node *y = merge_tree->get_border_node_lroot(v_t->at(i)->boundary_parent);
        if(y == NULL){
            y = merge_tree->get_border_node_lroot(v_t->at(i)->ptr_node->global_idx);
        }
                
        if(verbose){
            std::cout << "____________________________________________________________\n";
            std::cout << "merging: " << v_ret->at(i)->ptr_node->global_idx ;
            std::cout << " (lroot:" << x->ptr_node->global_idx << ")\n";
            std::cout << "   with: " << v_t->at(i)->ptr_node->global_idx;
            std::cout << " (lroot:" << v_t->at(i)->boundary_parent << ")\n";
            std::cout << "ret_tree:" << ret_tree->lroot_to_string() << "\n";
            
            std::cout << "ret_tree (attributes):" << ret_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
            std::cout << "============================================================\n";
            std::cout << "merge_tree:" << merge_tree->lroot_to_string() << "\n";
            
            std::cout << "merge_tree (attributes):" << ret_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
            std::cout << "____________________________________________________________\n";
        }  
        ret_tree->merge_branches(x, y, accumulated);

    }
    ret_tree->combine_borders(this, t, d);


    if(verbose){
        std::cout << "____________________________________________________________\n";
        std::cout << "this new tree:\n";//" << this->lroot_to_string() << "\n";
        this->print_tree();
        std::cout << "Atributes:\n";
        std::cout << this->lroot_to_string(BOUNDARY_ATTR) << "\n";
        std::cout << this->lroot_to_string(BOUNDARY_GVAL) << "\n";
        std::cout << "____________________________________________________________\n";
        std::cout << "merge new tree:\n";//" << t->lroot_to_string() << "\n";
        merge_tree->print_tree();
        std::cout << "Atributes:\n";
        std::cout << merge_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
        std::cout << merge_tree->lroot_to_string(BOUNDARY_GVAL) << "\n";
        std::cout << "____________________________________________________________\n";
        std::cout << "ret_tree new tree:\n";// << ret_tree->lroot_to_string() << "\n";
        ret_tree->print_tree();
        std::cout << "Atributes:\n";
        std::cout << ret_tree->lroot_to_string(BOUNDARY_ATTR) << "\n";
        std::cout << ret_tree->lroot_to_string(BOUNDARY_GVAL) << "\n";
        std::cout << "____________________________________________________________\n";
    } 

    delete merge_tree;
    if(verbose){
        std::cout << "ret_tree:" << ret_tree->lroot_to_string() << "\n";
        std::cout << "gval:" << ret_tree->lroot_to_string(BOUNDARY_GVAL) << "\n";
        std::cout << "attributes:" << ret_tree->lroot_to_string(BOUNDARY_ATTR) << "\n________________\n";
    }
    
    return ret_tree;
}


void boundary_tree::update(boundary_tree *merged){
    auto new_tree = merged->boundary_tree_lroot;
    for(auto node: *new_tree){
        if(verbose)
            std::cout << "updating " << node.first << " \n";
        if(node.second->border_lr == NO_BORDER_LEVELROOT){
            //does not need to change levelroot of the node
            if(this->boundary_tree_lroot->find(node.first) != this->boundary_tree_lroot->end()){
                // if node is in boundary tree just update this attribute
                this->boundary_tree_lroot->at(node.first)->ptr_node->attribute = node.second->ptr_node->attribute;
            }
        }else if (this->boundary_tree_lroot->find(node.first) != this->boundary_tree_lroot->end()){
            //node levelroot update on merge. so, we need to update its global parent and attributes
            auto x = merged->get_border_node_lroot(node.second->border_lr);
            auto this_node = this->get_border_node_lroot(node.first);
            this_node->ptr_node->attribute = x->ptr_node->attribute;
            this_node->border_lr = x->ptr_node->global_idx;
            if(verbose) {
                std::cout << "node:" << node.first << " attributes changing to: " 
                          << x->ptr_node->attribute << " that came from:" << x->ptr_node->global_idx << "\n";

            }
            //this->add_lroot_tree(x, true, true); 
        }
    
        if(verbose){
            std::cout << node.first << " updated \n_____________________\n";
        }
    }
    std::cout << "\n";
}

uint64_t boundary_tree::index_of(uint32_t l, uint32_t c){
    return l * this->w + c;
}

std::tuple<uint32_t, uint32_t> boundary_tree::lin_col(uint64_t index){
    return std::make_tuple(index / this->w, index % this->w);
}

std::string boundary_tree::lroot_to_string(enum boundary_tree_field f){
    std::ostringstream ss;
    for(auto bn: *(this->boundary_tree_lroot)){
        if(f == BOUNDARY_PARENT){
            ss << "(" << bn.first << ", " << bn.second->boundary_parent << ")";
        }else if(f == BOUNDARY_BORDER_LR){
            ss << "(" << bn.first << ", glr: " << bn.second->border_lr 
               << ", llr: " << bn.second->boundary_parent << ")";            
        }else if(f == BOUNDARY_LABEL){
            ss << "(" << bn.first << ", " << bn.second->ptr_node->label << ")"; 
        }else if(f == MAXTREE_IDX){
            ss << "(" << bn.first << ", " << bn.second->ptr_node->idx << ")";
        }else if(f == BOUNDARY_GVAL){
            ss << "(" << bn.first << ", " << (int)bn.second->ptr_node->gval << ")";
        }else if(f==BOUNDARY_GLOBAL_IDX){
            ss << "(" << bn.first << ", " << bn.second->ptr_node->global_idx << ")";
        }else if(f==BOUNDARY_ATTR){
            ss << "(n:" << bn.first << ", a:" << bn.second->ptr_node->attribute << ")";
        }
        ss << " ";
    }
    return ss.str();
}

std::string boundary_tree::border_to_string(enum boundary_tree_field f){
    
    std::ostringstream ss;
    for(int i=0; i<NamesBordersVector.size();i++){
        ss << fill(NamesBordersVector[i], 16) << ": ";
        auto v = *(this->border_elements->at(i));
        /*for(auto pairs: v){
            auto bn = pairs.second;*/
        for(auto bn: v){
            if(f == BOUNDARY_PARENT){
                ss << bn->boundary_parent;
            }else if(f == BOUNDARY_BORDER_LR){
                if(bn->border_lr != NO_BORDER_LEVELROOT){
                    ss << bn->border_lr;
                }else{
                    ss << bn->boundary_parent;
                }           
            }else if(f == MAXTREE_IDX){
                ss << bn->ptr_node->idx;
            }else if(f == BOUNDARY_GVAL){
                ss << bn->ptr_node->gval;
            }else if(f==BOUNDARY_GLOBAL_IDX){
                ss << bn->ptr_node->global_idx;
            }
            ss << " ";
        }
        ss << "\n";
    }
    return ss.str();
}


void boundary_tree::print_tree(enum boundary_tree_field lrootf, enum boundary_tree_field borderf){
    std::cout << "\n____________________________________________________\n";
    std::cout << ">>>>>>>>> All borders: <<<<<<<<<<<\n" << this->border_to_string(borderf);
    std::cout << ">>>>>>>>> Tree: <<<<<<<<<\n" << this->lroot_to_string(lrootf);
    std::cout << "\n_____________________________________________________\n";
}