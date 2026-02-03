#include <unordered_map>
#include <string>

#include "src/hps.h"

#include "const_enum_define.hpp"
#include "bag_of_task.hpp"
#include "tasks.hpp"

#ifndef __WORKERS_HPP__
#define __WORKERS_HPP__

extern std::pair<uint32_t, uint32_t> GRID_DIMS;

bool inside_rectangle(std::pair<uint32_t, uint32_t> c, std::pair<uint32_t, uint32_t> r);
std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t);


class worker{
    private:
        uint16_t id;
        std::unordered_map<std::string, double> *attr;
        bool busy;
    public:
        worker(uint16_t id, std::unordered_map<std::string, double> *attr);
        worker();
        // ~worker();
        void set_attr(std::string, double val);
        //virtual void run() = 0;
        Tprocess_power get_process_power();

        bool operator<(worker &r);
        bool operator>(worker &r);
        bool operator==(worker &r);
        
        void print();

        template <class B>
        void serialize(B &buf) const{
            buf << (*(this->attr)) << this->id;
        }

        template <class B>
        void parse(B &buf){
            buf >> (*(this->attr)) >> this->id;
        }

        //get a task from bag and compute maxtree of the tile of this task
        void maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, 
                          bag_of_tasks<maxtree_task *> &max_trees);

        void get_boundary_tree(bag_of_tasks<maxtree_task *> &maxtrees,
                               bag_of_tasks<boundary_tree_task *> &boundary_trees, 
                               bag_of_tasks<maxtree_task *>  &maxtree_dest);

        void search_pair(bag_of_tasks<boundary_tree_task *> &btrees_bag, 
                         bag_of_tasks<merge_btrees_task *> &merge_bag);

        void merge_local(bag_of_tasks<merge_btrees_task *> &merge_bag, 
                         bag_of_tasks<boundary_tree_task *> &btrees_bag);
        
        void update_filter(bag_of_tasks<maxtree_task *> &src,
                           bag_of_tasks<maxtree_task *> &dest,
                           boundary_tree *global_bt,
                           Tattribute lambda);
};

#endif