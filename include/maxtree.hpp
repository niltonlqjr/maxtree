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
        std::vector<double> adjacents;


    public:
        component(uint64_t id = -1, std::vector<int> p = std::vector<int>(), int parent = -1, int attribute=0);
        std::string to_string();
        void insert_pixel(int p);
        int id;
        std::vector<int> get_pixels_index();
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
        // std::unordered_map<int, maxtree_node*> *data;
        std::vector<maxtree_node *> *data;
        std::unordered_map<double, std::vector<component>> components;
        std::unordered_map<double, std::mutex> threshold_locks;
        std::mutex data_lock;
        
    public:
        
        int h;
        int w;
        maxtree(int h, int w);
        // maxtree(std::unordered_map<int, maxtree_node*> *data, int h, int w);

        maxtree(std::vector<maxtree_node*> *data, int h, int w);
        
        int index_of(int i, int j);
        void fill_from_VImage(vips::VImage &img, bool verbose=false);
        void fill_from_VRegion(vips::VRegion &reg_in, uint32_t base_h, uint32_t base_w, bool verbose=false);
        void insert_component(std::vector<int> component, int parent, double threshold, uint64_t id=-1);
        void insert_component(component c, double threshold);
        std::vector<component> components_at(double threshold);
        std::vector<double> all_thresholds();
        
        std::string to_string(enum maxtee_node_field field = PARENT,bool colored = true, int spaces = 5);

        unsigned long long int get_size();
        
        maxtree_node *at_pos(int h, int w);
        maxtree_node *at_pos(int index);
        // std::unordered_map<int, maxtree_node*> *get_data();

        std::vector<maxtree_node*> *get_data();

        std::vector<maxtree_node*> get_neighbours(int pixel, int con=4);
        //std::vector<maxtree_node*> get_neighbours(int l, int c, int con=4);
        std::tuple<int,int> lin_col(int index);
        void compute_sequential_iterative();
        void filter(Tattribute a);
};

#endif