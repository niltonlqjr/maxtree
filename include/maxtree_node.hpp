#include <string>

#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

class maxtree_node{
    public:
        int parent;
        int label;
        unsigned int idx;
        double gval;

        maxtree_node(double g, unsigned int i);
    

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




