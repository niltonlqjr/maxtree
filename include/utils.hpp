#include <vips/vips8>
#include <iostream>
#include <vector>
#include <tuple>
#include <ostream>
#include <stack>
#include <queue>
#include <unordered_map>
#include <fstream>
#include <algorithm>

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

std::unordered_map<std::string, std::string> *parse_config(char arg[]);

void print_unordered_map(std::unordered_map<std::string, std::string> *m);

unsigned int index_of(unsigned int l, unsigned int c, int h, int w);

std::tuple<unsigned int, unsigned int> lin_col(int index, int h, int w);

void print_VImage_band(vips::VImage *in, int band = 0);

void print_labels(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false);

void print_matrix(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false);

void print_stack(std::stack<maxtree_node*> s);

bool is_blank(std::string s, std::vector<char> b = {' ', '\t', '\n', '\r'});

std::string ltrim(std::string s, const std::string b = std::string(" \t\n\r"));

std::string rtrim(std::string s, const std::string b = std::string(" \t\n\r"));

std::string fill(std::string s, int size);

std::vector<maxtree_node*> get_neighbours(maxtree_node *pixel, 
    std::vector<maxtree_node *> *t,
    unsigned int h, unsigned int w);

maxtree_node *min_gval(std::vector<maxtree_node*> *t);

void print_pq(std::priority_queue<maxtree_node*> pq);

#endif