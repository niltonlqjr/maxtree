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
        double gval; // gray value
        //bool in_tree;
        uint64_t maxtree_idx; //index of node on local tile
        uint64_t global_idx; // index of node on the whole image
        int64_t maxtree_levelroot; // 
        uint64_t origin; // index of node on the border that added the boundary tree branch
        int64_t boundary_parent; // parent of the node in boundary tree
        boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin,
                      uint64_t global_idx, int64_t bound_parent = -1);
        boundary_node(maxtree_node *n, uint64_t origin,
                      int64_t bound_parent = -1);


};

class boundary_tree{
    private:
        std::vector< std::unordered_map<uint64_t, boundary_node *> *> *border_elements;
        std::unordered_map<uint64_t, boundary_node*> *boundary_tree_lroot;
    public:
        uint32_t h;
        uint32_t w;
        uint32_t grid_i;
        uint32_t grid_j;
        boundary_tree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j);
        boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j);
        ~boundary_tree();
        void insert_element(boundary_node &n, enum borders b, int64_t origin=-1);
        boundary_node *get_border_node_lroot(int64_t maxtree_idx);
        std::string to_string(enum boundary_tree_field f=BOUNDARY_PARENT);
        void merge(boundary_tree *t, enum merge_directions d, uint8_t connection = 4);
        void add_lroot_tree(maxtree_node *tn, int64_t origin, std::vector<maxtree_node*> *maxtree_data);
        bool insert_lroot(boundary_node *n);
        bool is_root(uint64_t n_idx);
        void merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node);
};


#endif