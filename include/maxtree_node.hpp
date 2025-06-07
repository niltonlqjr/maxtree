#include <string>

#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

enum maxtee_node_field {
    PARENT, LABEL, IDX, GVAL, LEVELROOT
};

#define Tattribute unsigned long long int
#define Tattr_default 0

class maxtree_node{
    public:
        long long int parent;
        long long int label;
        unsigned long long int idx;
        double gval;
        bool visited;
        Tattribute attribute;

        maxtree_node(double g, unsigned long long int i, Tattribute attr = Tattr_default);
    

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




