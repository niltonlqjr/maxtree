#include <map>
#include <tuple>
#include "utils.hpp"
#include "maxtree_node.hpp"

#ifndef __MAXTREE_HPP__
#define __MAXTREE_HPP__

class maxtree{
    private:
        std::map<int, maxtree_node*> *data;
        int h;
        int w;
        int index_of(int l, int c);
         
        
    public:
        maxtree(int h, int w);
        maxtree(std::map<int, maxtree_node*> *data, int h, int w);
        maxtree_node *at_pos(int h, int w);
        maxtree_node *at_pos(int index);
        std::vector<maxtree_node*> get_neighbours(int pixel);
        std::vector<maxtree_node*> get_neighbours(int l, int c);
        std::tuple<int,int> lin_col(int index);
};

#endif