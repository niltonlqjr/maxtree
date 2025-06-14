#include <vector>
#include <cinttypes>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include "maxtree_node.hpp"

#ifndef __BOUNDARY_TREE__
#define __BOUNDARY_TREE__

enum boundary_tree_field{
    BOUNDARY_PARENT, MAXTREE_IDX, BOUNDARY_IDX, BOUNDARY_GVAL, BOUNDARY_LEVELROOT
};

class boundary_node{
    public:
        double gval;
        //bool in_tree;
        uint64_t maxtree_idx;
        //uint64_t boundary_idx;
        int64_t boundary_parent;
        int64_t boundary_levelroot;
        uint64_t origin;
        boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin, int64_t boundary_parent=-1,int64_t boundary_levelroot=-1);
        boundary_node(maxtree_node *n, uint64_t origin, int64_t boundary_parent=-1,int64_t boundary_levelroot=-1);

};

class boundary_tree{
    private:
        std::unordered_map<uint64_t, boundary_node *> *border_elements;
    public:
        boundary_tree();
        boundary_tree(std::unordered_map<uint64_t, boundary_node *> *border_elements);
        ~boundary_tree();
        bool insert_element(boundary_node &n, int64_t origin=-1);
        boundary_node *get_border_node(int64_t maxtree_idx);
        std::string to_string(enum boundary_tree_field f=BOUNDARY_PARENT);
        void merge(boundary_tree *t);
        void add_parents(maxtree_node *tn, std::vector<maxtree_node*> *maxtree_data);
};


#endif