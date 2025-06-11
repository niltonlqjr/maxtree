#include <string>
#include <cinttypes>
#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

enum maxtee_node_field {
    PARENT, LABEL, IDX, GVAL, LEVELROOT, ATTRIBUTE
};

#define Tattribute uint64_t
#define Tattr_default 1

class maxtree_node{
    public:
        int64_t parent;
        int64_t label;
        uint64_t idx;
        double gval;
        bool visited;
        Tattribute attribute;

        maxtree_node(double g, uint64_t i, Tattribute attr = Tattr_default);
        void compute_attribute(Tattribute);

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




