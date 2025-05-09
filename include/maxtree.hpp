#include <unordered_map>
#include <string>
#include <tuple>
#include <mutex>
#include "utils.hpp"
#include "maxtree_node.hpp"

#ifndef __MAXTREE_HPP__
#define __MAXTREE_HPP__


class component{
    private:
        std::vector<int> pixels;
        int parent;
        int attribute;
    public:
        component(std::vector<int> p = std::vector<int>(), int parent = -1, int attribute=0);
        std::string to_string();
            
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
        std::unordered_map<double, std::vector<component>> components;
        std::unordered_map<double, std::mutex> threshold_locks;
        std::mutex data_lock;
        
    public:
        int h;
        int w;
        maxtree(int h, int w);
        maxtree(std::unordered_map<int, maxtree_node*> *data, int h, int w);
        
        void fill_from_VImage(vips::VImage img);
        void insert_component(std::vector<int> component, int parent, double threshold);
        std::vector<component> components_at(double threshold);
        std::vector<double> all_thresholds();

        std::string to_string(enum maxtee_node_field field = PARENT, int spaces = 5);

        maxtree_node *at_pos(int h, int w);
        maxtree_node *at_pos(int index);
        std::vector<maxtree_node*> get_neighbours(int pixel);
        std::vector<maxtree_node*> get_neighbours(int l, int c);
        std::tuple<int,int> lin_col(int index);
};

#endif