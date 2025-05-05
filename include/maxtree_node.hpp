#include <string>

#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

enum maxtee_node_field {
    PARENT, LABEL, IDX, GVAL
};

class maxtree_node{
    public:
        long long int parent;
        long long int label;
        unsigned long long int idx;
        double gval;

        maxtree_node(double g, unsigned long long int i);
    

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




