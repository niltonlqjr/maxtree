#include <unordered_map>
#include <string>
#include <iostream>

#include "src/hps.h"
#include "zmq.hpp"

#include "const_enum_define.hpp"
#include "bag_of_task.hpp"
#include "tasks.hpp"
#include "message.hpp"

#ifndef __WORKERS_HPP__
#define __WORKERS_HPP__

extern std::pair<uint32_t, uint32_t> GRID_DIMS;

bool inside_rectangle(std::pair<uint32_t, uint32_t> c, std::pair<uint32_t, uint32_t> r);
std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t);


class worker{
    private:
        TWorkerIdx id;
        std::unordered_map<std::string, TWorkerAttr> *attr;
        bool busy, connected;
        std::string manager; // address of manager
        std::string self_address; // self address
        zmq::context_t context;
        zmq::socket_t sock;
    public:
        
        worker(TWorkerIdx id, std::string manager = "", std::string address = "", std::unordered_map<std::string, TWorkerAttr> *attr = nullptr);
        worker(worker &w);
        worker();
        // ~worker();
        void set_attr(std::string s, TWorkerAttr val);
        //virtual void run() = 0;
        TWorkerAttr get_attr(std::string s);
        TWorkerIdx get_index();

        Tprocess_power get_process_power();
        
        bool operator<(worker &r);
        bool operator>(worker &r);
        bool operator==(worker &r);

        std::string to_string();
        void print();

        template <class B>
        void serialize(B &buf) const{
            buf << (*(this->attr)) << this->id;
        }

        template <class B>
        void parse(B &buf){
            buf >> (*(this->attr)) >> this->id;
        }
        
        void update_index(TWorkerIdx new_idx);

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
        
        /* sign up for server address stored on this->manager*/
        void registry();

        /* sign up for server address server_addr*/
        void registry_at(std::string server_addr);

        /* request one task to server. if sock is nullptr, then
        this->sock is used, otherwise, use sock passed as arg */
        message request_work();
        bool send_answer(message &m);

        void connect();
        void disconnect();
        
        void send_boundary_tree(boundary_tree *bt);
};

#endif