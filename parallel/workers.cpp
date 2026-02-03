#include "workers.hpp"


/* check if pair of coordinates c is inside the rectangle r (starting at origin (0,0) and 
    ending in position (r.first-1, r.second-1) */
bool inside_rectangle(std::pair<uint32_t, uint32_t> c, std::pair<uint32_t, uint32_t> r){
    if(c.first >= r.first || c.first < 0 || c.second >= r.second || c.second < 0){
        return false;
    }
    return true;
}

std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t){
    return t->index;
}


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

// worker::~worker(){
//     delete this->attr;
// }

void worker::set_attr(std::string attr_name, double attr_val){
    (*this->attr)[attr_name] = attr_val;
}

Tprocess_power worker::get_process_power(){
    return this->attr->at("MHZ") * this->attr->at("NUMPROC");
}

bool worker::operator<(worker &r) {
    return this->get_process_power() < r.get_process_power();
}

bool worker::operator>(worker &r){
    return this->get_process_power() > r.get_process_power();
}

bool worker::operator==(worker &r){
    return this->get_process_power() == r.get_process_power();
}

void worker::print(){
    std::cout << "local id:" << this->id << " -- ";
    std::cout << "attributes:\n";
    for(auto elem: *this->attr){
        std::cout << "(" << elem.first << "," << elem.second << ") ";
    }
    std::cout << "\n";

}

void worker::get_boundary_tree(bag_of_tasks<maxtree_task *> &maxtrees,
                               bag_of_tasks<boundary_tree_task *> &boundary_trees, 
                               bag_of_tasks<maxtree_task *>  &maxtree_dest){
    bool got_task;
    maxtree_task *mtt;
    boundary_tree_task *btt;
    bag_of_tasks<maxtree_task *> maxtree_aux;
    // while(maxtrees.is_running()){
        got_task = maxtrees.get_task(mtt);
        if(got_task){
            this->busy = true;
            // std::string stmt = std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) +"\n";
            // stmt += mtt->mt->to_string(GLOBAL_IDX,false) + "\n";
            // std::cout << stmt;
            btt = new boundary_tree_task(mtt, std::make_pair<uint32_t,uint32_t>(0,1));
            boundary_trees.insert_task(btt);
            maxtree_dest.insert_task(mtt);
            this->busy = false;
        }
    // }

}

void worker::maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, bag_of_tasks<maxtree_task *> &max_trees){
    bool got_task;
    input_tile_task *t;
    maxtree_task *mt;
    // while(bag_tiles.is_running()){
        if(verbose){
            std::ostringstream os("");
            os << "thread " << this->id << " trying to get task\n";
            std::string s = os.str();
            std::cout << s;
        }
        got_task=bag_tiles.get_task(t);
        if(got_task){
            this->busy = true;
            if(verbose){
                std::ostringstream os("");
                os << "worker " <<  this->id << " got task " << t->i << ", " << t->j << " to calculate maxtree\n";
                std::string s = os.str();
                std::cout << s;
            }
            mt = new maxtree_task(t);
            max_trees.insert_task(mt);
            this->busy = false;
        }
        if(verbose){
            std::ostringstream os("");
            os << "thread " << this->id << " couldnt get task\n";
            std::string s = os.str();
            std::cout << s; 
        }
    // }
}

void worker::search_pair(bag_of_tasks<boundary_tree_task *> &btrees_bag, 
                         bag_of_tasks<merge_btrees_task *> &merge_bag){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    // while(btrees_bag.is_running() || !btrees_bag.get_num_task() > 1){
        bool got = btrees_bag.get_task(btt);
        if(got){
            this->busy = true;
            if(btt->nb_distance.first == 0){
                merge_dir = MERGE_VERTICAL_BORDER;
            }else if(btt->nb_distance.second == 0){
                merge_dir = MERGE_HORIZONTAL_BORDER;
            }else{
                std::cerr << "merge distance invalid: " << int_pair_to_string(btt->nb_distance) << "\n";
                exit(EXIT_FAILURE);
            }
            // s = "got tree: " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + 
            //     " with distance: "+ int_pair_to_string(btt->nb_distance) +"\n";
            // std::cout << s;
            if(merge_dir == MERGE_VERTICAL_BORDER){
                if(btt->bt->grid_j % int_pow(2,btt->nb_distance.second) == 0){
                    nb_direction = NB_AT_RIGHT;
                }else{
                    nb_direction = NB_AT_LEFT;
                }
            }else if(merge_dir == MERGE_HORIZONTAL_BORDER){
                if(btt->bt->grid_i % int_pow(2,btt->nb_distance.first) == 0){
                    nb_direction = NB_AT_BOTTOM;
                }else{
                    nb_direction = NB_AT_TOP;
                }
            }
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
                try{
                    auto got_n = btrees_bag.get_task_by_field<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    
                    if(!got_n){
                        btrees_bag.insert_task(btt);
                        // std::string s = "no neighbor" + std::to_string(btt->index.first) + "," + std::to_string(btt->index.second) + "\n";
                        // std::cout << s;
                    }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){

                        if((merge_dir == MERGE_VERTICAL_BORDER) && (btt->bt->grid_j > n->bt->grid_j) ||
                           (merge_dir == MERGE_HORIZONTAL_BORDER) && (btt->bt->grid_i > n->bt->grid_i)){
                            if(verbose){
                                std::string s = "swap: " + int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + "\n";
                                std::cout << s;
                            }
                            aux = btt;
                            btt = n;
                            n = aux;
                        }
                        if(merge_dir == MERGE_HORIZONTAL_BORDER&& btt->bt->border_elements->at(BOTTOM_BORDER)->size() != n->bt->border_elements->at(TOP_BORDER)->size()){
                            std::string s = int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + " borders differs in size\n";
                            s += "distances:" + int_pair_to_string(btt->nb_distance) + " and " + int_pair_to_string(n->nb_distance) + "\n";
                            std::cout << s;
                        }

                        // s = "creating task with btt " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + "   ";
                        // s += "n: "  + std::to_string(n->bt->grid_i) + "," + std::to_string(n->bt->grid_j) + "distance ";
                        // s += int_pair_to_string(btt->nb_distance) +"\n";
                        // // std::cout << s;
                        auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                        merge_bag.insert_task(new_merge_task);
                    }else{
                        if(verbose){
                            std::string s = "invalid distance:" + int_pair_to_string(btt->index) + " ->" + int_pair_to_string(btt->nb_distance) + "\n";
                            s+="invalid distance:" + int_pair_to_string(n->index) + " ->" + int_pair_to_string(n->nb_distance) + "\n=======\n";
                            std::cout << s;
                        }
                        btrees_bag.insert_task(n);
                        btrees_bag.insert_task(btt);
                    }
                }
                catch(std::runtime_error &e){
                    std::cerr << e.what();
                    // s = "pair of " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + " not found ";
                    // s += std::to_string(idx_nb.first) + "," + std::to_string(idx_nb.second) + "\n";
                    // std::cout << s;
                    btrees_bag.insert_task(btt);
                }catch(std::out_of_range &r){
                    std::cerr << "try to access an out of range element\n";
                }
            }else if (inside_rectangle(btt->index, GRID_DIMS)){ // the neighbor of btt is not inside the grid (it does not exist, so go to next merge)
                if(btt->nb_distance.first == 0){
                    if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " line distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.second *= 2;
                    }else{ // there is no more neighbor on this line to merge, so this line must be merged with the other lines
                        
                        btt->nb_distance.first = 1; 
                        btt->nb_distance.second = 0;
                    }
                }else if(btt->nb_distance.second == 0){
                    if(btt->nb_distance.first < GRID_DIMS.first){
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " column distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.first *= 2;
                    }
                }
                btrees_bag.insert_task(btt);   
            }
            this->busy = false;
        }
    // }
    if(verbose) std::cout << "end worker search pair\n";
}


void worker::merge_local(bag_of_tasks<merge_btrees_task *> &merge_bag, bag_of_tasks<boundary_tree_task *> &btrees_bag){
    merge_btrees_task *mbt;
    boundary_tree_task *btt;
    boundary_tree *nbt;
    std::pair<uint32_t, uint32_t> dist;
    std::string s;
    while(merge_bag.is_running() || !merge_bag.empty()){

        bool got_mt = merge_bag.get_task(mbt);
        if(got_mt){
            if(verbose){
                s = "-------------------TREE 1------------------\n";
                // mbt->bt1->print_tree();
                s+=mbt->bt1->border_to_string();
                s+=mbt->bt1->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                s = "-------------------TREE 2------------------\n";
                // mbt->bt2->print_tree();
                s+=mbt->bt2->border_to_string();
                s+=mbt->bt2->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                // std::cout << s;
                std::cout << s;
                s = "task will merge: " + std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + " \n";
                std::cout << s;
            }
            nbt = mbt->execute();
            
            dist.second = (mbt->bt2->grid_j - mbt->bt1->grid_j) * 2;
            dist.first = (mbt->bt2->grid_i - mbt->bt1->grid_i) * 2;
            if(dist.second >= GRID_DIMS.second){
                dist.first = 1;
                dist.second = 0;
            }else if(dist.second == 0 && dist.first >= GRID_DIMS.first){
                if(verbose){
                    std::string s = "ending  merge --- dist:" + int_pair_to_string(dist) + " " ;
                    s += std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                    s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + "\n";
                    std::cout << s;
                }
                merge_bag.notify_end();
                btrees_bag.notify_end();
            }

            if(verbose) nbt->print_tree();
            btt = new boundary_tree_task(nbt, dist);
            if(verbose){
                s += " task inserted with index:" + std::to_string(btt->bt->grid_i) + ", " + std::to_string(btt->bt->grid_j);
                s += " and distance " + int_pair_to_string(btt->nb_distance) + "\n";
                std::cout << s;
            }
            btrees_bag.insert_task(btt);
        }
    }
    if(verbose)std::cout << "end worker local merge\n";
}

void worker::update_filter(bag_of_tasks<maxtree_task *> &src, bag_of_tasks<maxtree_task *> &dest, boundary_tree *global_bt, Tattribute lambda){
    bool got_task;
    maxtree_task *mtt;
    std::string s;
    if(verbose) std::cout << "worker update\n";
    // while(src.is_running()){
        got_task = src.get_task(mtt);
        // s = "updating (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") \n";
        // std::cout << s;
        if(got_task){
            this->busy = true;
            mtt->mt->update_from_boundary_tree(global_bt);
            dest.insert_task(mtt);
            mtt->mt->filter(lambda, global_bt);
            this->busy = false;
        }
        if(verbose) {
            s = "task of grid (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") update\n";
            std::cout << s;
        }
    // }
}


// template <class B>
// void worker::serialize(B &buf) const {
//     buf << (*(this->attr)) << this->id;
// }

// template <class B>
// void worker::parse(B &buf){
//     buf >> (*(this->attr)) >> this->id;
    
// }
