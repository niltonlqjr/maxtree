#include <unordered_map>
#include <unordered_map>
#include <tuple>
#include "utils.hpp"
#include "maxtree_node.hpp"

#ifndef __MAXTREE_HPP__
#define __MAXTREE_HPP__


class component{
    private:
        std::vector<int> pixels;
        int attribute;
    public:
        component(std::vector<int> p = std::vector<int>(), int attribute=0);
        //compare with component
/* 
        bool operator>(const component &r);
        bool operator<(const component &r);
        bool operator==(const component &r);

        //compare with integer
        bool operator>(const int &r);
        bool operator<(const int &r);
        bool operator==(const int &r);
 */

};

class maxtree{
    private:
        std::unordered_map<int, maxtree_node*> *data;
        int index_of(int l, int c);
        std::unordered_map<int, std::vector<component>> components;
        
    public:
        int h;
        int w;
        maxtree(int h, int w);
        maxtree(std::unordered_map<int, maxtree_node*> *data, int h, int w);
        
        void insert_component(std::vector<int> component, double threshold);
        
        maxtree_node *at_pos(int h, int w);
        maxtree_node *at_pos(int index);
        std::vector<maxtree_node*> get_neighbours(int pixel);
        std::vector<maxtree_node*> get_neighbours(int l, int c);
        std::tuple<int,int> lin_col(int index);
};

#endif