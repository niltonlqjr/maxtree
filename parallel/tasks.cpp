#include "tasks.hpp"

// extern std::vector<std::pair<int32_t, int32_t>> NEIGHBOR_DIRECTION;

#ifndef __GRID_DIMS__
#define __GRID_DIMS__

std::pair<uint32_t, uint32_t> GRID_DIMS;

#else
extern std::pair<uint32_t, uint32_t> GRID_DIMS;
#endif

bool operator<(comparable_task &l, comparable_task &r){
    return l.size() < r.size();
}

 bool operator>(comparable_task &l, comparable_task &r){
    return l.size() > r.size();
}

 bool operator==(comparable_task &l,  comparable_task &r){
    return l.size() == r.size();
}

/* const char *task_exception::what() const {
    return this->msg.c_str();
}

task_exception::task_exception(std::string &m){
    this->msg = m;
}
 */

// Input tile task
input_tile_task::input_tile_task(uint32_t i, uint32_t j, uint32_t nb_rt, uint32_t nb_rl){
    this->i = i;
    this->j = j;
    this->tile_columns = 0;
    this->tile_lines = 0;
    this->reg_left = 0;
    this->reg_top = 0;
    this->noborder_rt = nb_rt;
    this->noborder_rl = nb_rl;
}

input_tile_task::input_tile_task(uint32_t i, uint32_t j){
    this->i = i;
    this->j = j;
    this->tile_columns = 0;
    this->tile_lines = 0;
    this->reg_left = 0;
    this->reg_top = 0;
    this->noborder_rt = 0;
    this->noborder_rl = 0;
}

input_tile_task::input_tile_task(std::pair<uint32_t, uint32_t> grid_idx){
    this->i = grid_idx.first;
    this->j = grid_idx.second;
    this->tile_columns = 0;
    this->tile_lines = 0;
    this->reg_left = 0;
    this->reg_top = 0;
    this->noborder_rt = 0;
    this->noborder_rl = 0;
}

uint64_t input_tile_task::size(){
    return this->tile_columns * this->tile_lines;
}

void input_tile_task::prepare(vips::VImage *img, uint32_t glines, uint32_t gcolumns){
                         
    uint32_t h,w;
    h=img->height();
    w=img->width();
    
    uint32_t h_trunc = h/glines;
    uint32_t w_trunc = w/gcolumns;

    uint32_t num_h_ceil = h%glines;
    uint32_t num_w_ceil = w%gcolumns;


    bool left, top, right, bottom;
    // uint32_t x = 0;
    uint32_t lines_inc, columns_inc; // original tiles (without borders) size variables
    //uint32_t reg_top, reg_left, tile_lines, tile_columns; // tiles used by algorithm (with border) size variables

    if(this->i < num_h_ceil){
        noborder_rt = this->i * (h_trunc+1);
    }else{
        noborder_rt = num_h_ceil + this->i * h_trunc;
    }

    if(this->j < num_w_ceil){
        noborder_rl= this->j * (w_trunc+1);
    }else{
        noborder_rl = num_w_ceil + this->j * w_trunc;
    }

    std::vector<bool> borders(4,false);
    
    lines_inc = this->i < num_h_ceil ? h_trunc +1 : h_trunc;

    // check which borders the tile must have.
    this->tile_lines = lines_inc;
    this->reg_top = noborder_rt;
    if(noborder_rt > 0){ //check if there is a top border
        this->tile_lines++;
        this->reg_top--;
        borders.at(TOP_BORDER) = true;
    }
    if(noborder_rt+lines_inc < h - 1){//check if there is a bottom border
        this->tile_lines++;
        borders.at(BOTTOM_BORDER) = true;
    }
    
    borders.at(LEFT_BORDER) = false;
    borders.at(RIGHT_BORDER) = false;
    columns_inc = this->j < num_w_ceil ? w_trunc+1 : w_trunc;
    this->tile_columns = columns_inc;
    this->reg_left = noborder_rl;
    if(noborder_rl > 0){//check if there is a left border
        this->tile_columns++;
        this->reg_left--;
        borders.at(LEFT_BORDER) = true;
    }
    if(noborder_rl+columns_inc < w - 1){//check if there is a right border
        this->tile_columns++;
        borders.at(RIGHT_BORDER) = true;
    }
    if(verbose)  std::cout << this->i << ", " << this->j << " real top:" << noborder_rt << " real left: " << noborder_rl 
                           << " lines:" << this->tile_lines << " cols:" << this->tile_columns << "\n";

    // create the class for tile representation
    this->tile = new maxtree(borders, this->tile_lines, this->tile_columns, this->i, this->j);
}

void input_tile_task::read_tile(vips::VImage *img){
    uint32_t h,w;
    h=img->height();
    w=img->width();
    
    uint32_t top_ini, left_ini;

    vips::VRegion reg = img->region(this->reg_left, this->reg_top, this->tile_columns, this->tile_lines);
    reg.prepare(this->reg_left, this->reg_top, this->tile_columns, this->tile_lines);
    
    if(verbose) std::cout << "reding:"<< this->i << ", " << this->j << " real top:" << this->reg_top << " real left: " << this->reg_left 
                          << " lines:" << this->tile_lines << " cols:" << this->tile_columns << "\n";

    this->tile->fill_from_VRegion(reg, this->reg_top, this->reg_left, h, w);
    vips_region_invalidate(reg.get_region());
}

// maxtree_task class

maxtree_task::maxtree_task(input_tile_task *t, bool copy) {
    if(copy){
        std::cerr << "not ready yet\n creating a maxtreetask with pointer instead of a copy\n";
        this->mt = t->tile;
    }else{
        this->mt = t->tile;
    }
    this->mt->compute_sequential_recursive();
}

uint64_t maxtree_task::size(){
    return this->mt->get_size();
}

//boundary_task class

boundary_tree_task::boundary_tree_task(maxtree_task *t, std::pair<uint32_t, uint32_t> nb_distance){
    this->bt = t->mt->get_boundary_tree();
    this->index = std::make_pair(this->bt->grid_i, this->bt->grid_j);
    this->nb_distance = nb_distance;
}

boundary_tree_task::boundary_tree_task(boundary_tree *t, std::pair<uint32_t, uint32_t> nb_distance){
    this->bt = t;
    this->index = std::make_pair(t->grid_i, t->grid_j);
    this->nb_distance = nb_distance;
}

boundary_tree_task::boundary_tree_task(){
    this->bt = nullptr;
    this->index = std::make_pair<uint32_t, uint32_t>(0, 0);
    this->nb_distance = std::make_pair<uint32_t, uint32_t>(0, 0);
}

uint64_t boundary_tree_task::size(){
    return this->bt->boundary_tree_lroot->size();
    // return this->index;
}

std::pair<uint32_t, uint32_t> boundary_tree_task::neighbor_idx(enum neighbor_direction direction){
    int32_t i_desloc = NEIGHBOR_DIRECTION[direction].first * this->nb_distance.first;
    int32_t j_desloc = NEIGHBOR_DIRECTION[direction].second * this->nb_distance.second;
    return std::make_pair(this->bt->grid_i + i_desloc, this->bt->grid_j + j_desloc); 
}

bool boundary_tree_task::can_merge_with(boundary_tree_task *btt2){
    return this->nb_distance == btt2->nb_distance;
}

bool boundary_tree_task::can_merge_with(boundary_tree_task &btt2){
    return this->nb_distance == btt2.nb_distance;
}


enum merge_directions boundary_tree_task::define_merge_direction(){
    if(this->nb_distance.first == 0){
        return MERGE_VERTICAL_BORDER;
    }else if(this->nb_distance.second == 0){
        return MERGE_HORIZONTAL_BORDER;
    }else{
        std::cerr << "merge distance invalid: " << int_pair_to_string(this->nb_distance) << "\n";
        std::cerr << "index: ";
        exit(EXIT_FAILURE);
    }
}

enum neighbor_direction boundary_tree_task::define_nb_direction(enum merge_directions merge_dir){
    enum neighbor_direction ret;
    if(merge_dir == MERGE_VERTICAL_BORDER){
        if(this->bt->grid_j % int_pow(2,this->nb_distance.second) == 0){
            ret = NB_AT_RIGHT;
        }else{
            ret = NB_AT_LEFT;
        }
    }else if(merge_dir == MERGE_HORIZONTAL_BORDER){
        if(this->bt->grid_i % int_pow(2,this->nb_distance.first) == 0){
            ret = NB_AT_BOTTOM;
        }else{
            ret = NB_AT_TOP;
        }
    }
    return ret;
}

merge_btrees_task::merge_btrees_task(boundary_tree *t1, boundary_tree *t2, enum merge_directions direction, std::pair<uint32_t, uint32_t> distance){
    if((direction == MERGE_VERTICAL_BORDER) && (t1->grid_j > t2->grid_j) ||
       (direction == MERGE_HORIZONTAL_BORDER) && (t1->grid_i > t2->grid_i)){
        auto aux = t1;
        t1 = t2;
        t2 = aux;
    }
    std::cout << "distance: " << int_pair_to_string(distance) << "\n";

    if(t1->grid_j+distance.second != t2->grid_j){
        std::string error_str = "non neighbours args for constructor of merge_btrees_task: " 
                                + this->bt1->index_to_string() + " and " 
                                + this->bt2->index_to_string() + " with ditance "
                                + int_pair_to_string(distance);
        throw std::runtime_error(error_str + "\n");
    }
    if(t1->grid_i+distance.first != t2->grid_i){
        std::string error_str = "non neighbours args for constructor of merge_btrees_task: " 
                                + this->bt1->index_to_string() + " and " 
                                + this->bt2->index_to_string() + " with ditance "
                                + int_pair_to_string(distance);
        throw std::runtime_error(error_str + "\n");
    }
    this->bt1 = t1;
    this->bt2 = t2;
    this->distance = distance;
    this->direction = direction;
}

merge_btrees_task::merge_btrees_task(boundary_tree_task *btt1, boundary_tree_task *btt2){
    enum merge_directions md1 = btt1->define_merge_direction();
    enum merge_directions md2 = btt2->define_merge_direction();
    enum merge_directions direction;
    std::pair<uint32_t, uint32_t> distance;
    if(md1 != md2){
        throw std::runtime_error("Trying to create a merge_btree_task between tiles in different steps\n");
    }else{
        direction = md1;
    }
    std::cout << "distances: btt1" << int_pair_to_string(btt1->nb_distance) << " btt2 " << int_pair_to_string(btt2->nb_distance)<< "\n";
    if(btt1->nb_distance != btt2->nb_distance){
        throw std::runtime_error("Invalid distance of tiles\n");
    }else{
        distance = btt1->nb_distance;
    }
    this->bt1 = btt1->bt;
    this->bt2 = btt2->bt;
    if((direction == MERGE_VERTICAL_BORDER) && (this->bt1->grid_j > this->bt2->grid_j) ||
       (direction == MERGE_HORIZONTAL_BORDER) && (this->bt1->grid_i > this->bt2->grid_i)){
        auto aux = this->bt1;
        this->bt1 = this->bt2;
        this->bt2 = aux;
    }
    if(this->bt1->grid_j+distance.second != this->bt2->grid_j){
        std::string error_str = "non neighbours args for constructor of merge_btrees_task: " 
                                + this->bt1->index_to_string() + " and " 
                                + this->bt2->index_to_string() + " with ditance "
                                + int_pair_to_string(distance);
        throw std::runtime_error(error_str + "\n");
    }
    if(this->bt1->grid_i+distance.first != this->bt2->grid_i){
        std::string error_str = "non neighbours args for constructor of merge_btrees_task: " 
                                + this->bt1->index_to_string() + " and " 
                                + this->bt2->index_to_string() + " with ditance "
                                + int_pair_to_string(distance);
        throw std::runtime_error(error_str + "\n");
    }
    this->distance = distance;
    this->direction = direction;
}

merge_btrees_task::merge_btrees_task(){
    this->bt1 = nullptr;
    this->bt2 = nullptr;
    this->distance = std::make_pair<uint32_t,uint32_t>(0,0);
    this->direction = MERGE_VERTICAL_BORDER;
}

uint64_t merge_btrees_task::size(){
    return this->bt1->boundary_tree_lroot->size() + this->bt2->boundary_tree_lroot->size();
}

boundary_tree *merge_btrees_task::execute(){
    boundary_tree *new_btree;
    // if(this->direction == MERGE_HORIZONTAL_BORDER){
    //     if(this->bt1->border_elements->at(BOTTOM_BORDER)->size() != this->bt2->border_elements->at(TOP_BORDER)->size()){
    //         return NULL;        
    //     }
    // }
    std::cout << "Merging: ";
    this->bt1->print_idx(" and ");
    this->bt2->print_idx();
   
    new_btree = this->bt1->merge(this->bt2, this->direction);

    new_btree->update_tree(new_btree);
    
    new_btree->compress_path();
    // delete this->bt1;
    // delete this->bt2;
    return new_btree;
}

merge_btrees_task::~merge_btrees_task(){
    // delete this->bt1;
    // delete this->bt2;
}
