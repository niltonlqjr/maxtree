#include <map>
#include <tuple>
#include "utils.hpp"
#include "maxtree_node.hpp"

#ifndef __MAXTREE_HPP__
#define __MAXTREE_HPP__

class maxtree{
    private:
        std::map<int, maxtree_node*> *data;
        int index_of(int l, int c);
        std::map<int, std::vector<int>> components;
        
    public:
        int h;
        int w;
        maxtree(int h, int w);
        maxtree(std::map<int, maxtree_node*> *data, int h, int w);
        
        void insert_component(std::vector<int> component);
        
        maxtree_node *at_pos(int h, int w);
        maxtree_node *at_pos(int index);
        std::vector<maxtree_node*> get_neighbours(int pixel);
        std::vector<maxtree_node*> get_neighbours(int l, int c);
        std::tuple<int,int> lin_col(int index);
};

#endif