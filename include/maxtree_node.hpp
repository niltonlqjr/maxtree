#include <string>

#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

enum maxtee_node_field {
    PARENT, LABEL, IDX, GVAL
};

#define Tattribute unsigned long long int
#define Tattr_default 0

class maxtree_node{
    public:
        long long int parent;
        long long int label;
        unsigned long long int idx;
        Tattribute a;
        double gval;
        bool visited;

        maxtree_node(double g, unsigned long long int i, Tattribute a = Tattr_default);
    

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




