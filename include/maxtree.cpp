#include "maxtree.hpp"

maxtree::maxtree(int h, int w){
    this->h = h;
    this->w = w;
}

maxtree::maxtree(std::map<int, maxtree_node*> *data, int h, int w){
    this->data = data;
    this->h=h;
    this->w=w;
}

maxtree_node *maxtree::at_pos(int l, int c){
    int idx = this->index_of(l, c);
    return this->data->at(idx);
}


maxtree_node *maxtree::at_pos(int index){
    return this->data->at(index);
}

std::vector<maxtree_node*> maxtree::get_neighbours(int pixel){
    std::vector<maxtree_node*> v;
    int idx, pl, pc;
    std::tie(pl, pc) = this->lin_col(pixel);
    if(pl >= 1){
        idx = index_of(pl-1, pc);
        v.push_back(this->data->at(idx));
    }
    if(pl < (unsigned int)h - 1){
        idx = index_of(pl+1, pc);
        v.push_back(this->data->at(idx));
    }
    if(pc >= 1){
        idx = index_of(pl, pc-1);
        v.push_back(this->data->at(idx));
    }
    if(pc < (unsigned int)w - 1){
        idx = index_of(pl, pc+1);
        v.push_back(this->data->at(idx));
    }
    return v;
}

std::vector<maxtree_node*> maxtree::get_neighbours(int pixel_line, int pixel_col){
    std::vector<maxtree_node*> v;
    int idx;
    if(pixel_line >= 1){
        idx = index_of(pixel_line-1, pixel_col);
        v.push_back(this->data->at(idx));
    }
    if(pixel_line < (unsigned int)h - 1){
        idx = index_of(pixel_line+1, pixel_col);
        v.push_back(this->data->at(idx));
    }
    if(pixel_col >= 1){
        idx = index_of(pixel_line, pixel_col-1);
        v.push_back(this->data->at(idx));
    }
    if(pixel_col < (unsigned int)w - 1){
        idx = index_of(pixel_line, pixel_col+1);
        v.push_back(this->data->at(idx));
    }
    return v;
}

int maxtree::index_of(int l, int c){
    return l * this->w + c;
}

std::tuple<int,int> maxtree::lin_col(int index){
    return std::make_tuple(index / this->w, index % this->w);
}