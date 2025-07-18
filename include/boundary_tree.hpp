#include <vector>
#include <cinttypes>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <sysexits.h>

#include "maxtree_node.hpp"

#ifndef __BOUNDARY_TREE__
#define __BOUNDARY_TREE__

#define NO_BORDER_LEVELROOT -1
#define NO_BOUNDARY_PARENT -1

#define Tboundary_tree_lroot std::unordered_map<uint64_t, boundary_node*>

enum boundary_tree_field{
    BOUNDARY_PARENT, MAXTREE_IDX, BOUNDARY_IDX, BOUNDARY_GVAL, BOUNDARY_LEVELROOT, BOUNDARY_GLOBAL_IDX
};


class boundary_node{
    public:
        //bool in_tree;
        int64_t border_lr; // levelroot of mergeed boundary trees (when nodes are merged)
        uint64_t origin; // index of node on the border that added the boundary tree branch
        int64_t boundary_parent; // parent of the node in boundary tree
        maxtree_node *ptr_node;
        
        /* boundary_node(double gval, uint64_t maxtree_idx, uint64_t origin,
                      uint64_t global_idx, Tattribute a = Tattr_default,
                      int64_t bound_parent = NO_BOUNDARY_PARENT,
                      int64_t border_lr = NO_BORDER_LEVELROOT); */
        boundary_node(maxtree_node *n, uint64_t origin,
                      int64_t bound_parent = NO_BOUNDARY_PARENT, 
                      int64_t border_lr = NO_BORDER_LEVELROOT);
        void accumulate_attr(boundary_node *merged);
        void accumulate_attr(Tattribute value);
};

class boundary_tree{
    private:
        //std::vector< std::unordered_map<uint64_t, boundary_node *> *> *border_elements;
        std::vector<std::vector<boundary_node *> *> *border_elements;
        std::unordered_map<uint64_t, boundary_node*> *boundary_tree_lroot;
        
    public:
        uint32_t h;
        uint32_t w;
        uint32_t grid_i;
        uint32_t grid_j;
        //int64_t border_lr;
        boundary_tree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j);
        /*boundary_tree(std::vector<std::unordered_map<uint64_t, boundary_node *>*> *border_elements,
             uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j);*/
        ~boundary_tree();
        /* insert the node at border structure (border_elements) */
        void insert_border_element(boundary_node &n, enum borders b, int64_t origin=-1);
        /* insert the node at boundary tree structure */
        bool insert_bnode_lroot_tree(boundary_node *n);
        /* get node with global_idx at tree structure (boundary_tree_lroot)*/
        boundary_node *get_border_node_lroot(int64_t global_idx);
        /* merge the calling tree with t */
        boundary_tree *merge(boundary_tree *t, enum merge_directions d, uint8_t connection = 4, bool verbose=false);
        /* add a levelroot to tree structure (boundary_tree_lroot) */
        void add_lroot_tree(maxtree_node *levelroot, int64_t origin, std::vector<maxtree_node*> *maxtree_data);
        /* add a levelroot to tree structure (boundary_tree_lroot) */
        void add_lroot_tree(boundary_node *levelroot, boundary_tree *t_levelroot, bool copy = false);
        /* check if a node with n_idx is root of the tree */
        bool is_root(uint64_t n_idx);
        /* merge two branches started at nodes this_node(from this tree) and t_node (from t tree) */
        void merge_branches(boundary_node *this_node, boundary_tree *t, boundary_node *t_node);
        /* get index given a position */
        uint64_t index_of(uint32_t i, uint32_t j);
        /* change vector of one border (top, left, bottom or right) */
        void change_border(std::vector<boundary_node *> *new_border, enum borders b);
        /* obtain the line and column of a given index */
        std::tuple<uint32_t,uint32_t> lin_col(uint64_t index);
        /* convert all border_elements to string  */
        std::string border_to_string(enum boundary_tree_field f=BOUNDARY_GLOBAL_IDX);
        /* print all nodes from the boundary_tree_lroot */
        std::string lroot_to_string(enum boundary_tree_field f=BOUNDARY_PARENT);
        /* return number of nodes in boundary_tree_lroot */
        uint64_t get_lroot_tree_size();
        /* return number of nodes in all borders*/
        uint64_t get_border_size();
};


#endif