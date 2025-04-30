#include <vips/vips8>
#include <iostream>
#include <vector>
#include <tuple>
#include <ostream>
#include <stack>
#include <queue>
#include "maxtree_node.hpp"

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

void label_components(std::vector<maxtree_node*> *mt);

struct cmp_maxtree_nodes{
    bool operator()(const maxtree_node* lhs, const maxtree_node* rhs) const{
        return lhs->gval < rhs->gval;
    }
};

std::ostream &operator<<(std::ostream &o, maxtree_node &n);

unsigned int index_of(unsigned int l, unsigned int c, int h, int w);

std::tuple<unsigned int, unsigned int> lin_col(int index, int h, int w);

void print_VImage_band(vips::VImage *in, int band = 0);

void print_labels(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false);

void print_matrix(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false);

void print_stack(std::stack<maxtree_node*> s);

std::string fill(std::string s, int size);

std::vector<maxtree_node*> get_neighbours(maxtree_node *pixel, 
    std::vector<maxtree_node *> *t,
    unsigned int h, unsigned int w);

maxtree_node *min_gval(std::vector<maxtree_node*> *t);

void print_pq(std::priority_queue<maxtree_node*> pq);

#endif