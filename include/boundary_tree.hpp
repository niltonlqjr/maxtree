#include <vector>
#include "maxtree_node.hpp"

#ifndef __BOUNDARY_TREE__
#define __BOUNDARY_TREE__


class boundary_node{
    public:
        double gval;
        bool in_tree;
        uint64_t maxtree_idx;
        uint64_t boundary_idx;
        int64_t boundary_parent;
        int64_t boundary_levelroot;
        boundary_node(double gval, uint64_t maxtree_idx, uint64_t boundary_idx, bool in_tree=false,
                      int64_t boundary_parent=-1,int64_t boundary_levelroot=-1);
        boundary_node(maxtree_node *n, uint64_t boundary_idx, bool in_tree=false,
                      int64_t boundary_parent=-1,int64_t boundary_levelroot=-1);

};

class boundary_tree{
    private:
        std::vector<boundary_node *> *border_elements;
    public:
        boundary_tree();
        boundary_tree(std::vector<boundary_node *> *border_elements);
        ~boundary_tree();
        void insert_element(boundary_node n);
};


#endif