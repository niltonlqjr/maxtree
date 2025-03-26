#include <map>
#include "utils.hpp"
#include "maxtree_node.hpp"

#ifndef __MAXTREE_HPP__
#define __MAXTREE_HPP__

class maxtree{
    private:
        std::map<int, maxtree_node*> *data;
        int h;
        int w;
    public:
        maxtree(int h, int w);
        maxtree(std::map<int, maxtree_node*> *data, int h, int w);
        maxtree_node *at_pos(int h, int w);
};

#endif