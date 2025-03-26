#include "maxtree_node.hpp"
#include "maxtree.hpp"
#include "utils.hpp"
#include <map>

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
    int idx = index_of(l, c, this->h, this->w);
    return this->data->at(idx);
}