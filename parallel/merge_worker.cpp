#include <vips/vips8>
// #include <vips/image.h> // to include VipsAccess enum

#include <iostream>
#include <vector>
#include <deque>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>
#include <zmq.hpp>

#include <sysexits.h>

#include "maxtree.hpp"
#include "maxtree_node.hpp"
#include "boundary_tree.hpp"
#include "const_enum_define.hpp"
#include "utils.hpp"
#include "bag_of_task.hpp"
#include "workers.hpp"
#include "tasks.hpp"
#include "src/hps.h"
#include "message.hpp"
#include "scheduler_of_workers.hpp"

using namespace vips;

bool print_only_trees, verbose;

// bool has_tiles, has_merges;
bool has_tasks;

extern std::pair<uint32_t, uint32_t> GRID_DIMS;

bag_of_tasks<input_tile_task *> G_bag_tiles;
bag_of_tasks<maxtree_task *> G_maxtrees;
// bag_of_tasks<boundary_tree_task *> boundary_trees;
// bag_of_tasks<maxtree_task *> maxtree_dest;

VipsAccess G_vips_access;

std::string G_server_ip, G_self_ip;
std::string G_server_port, G_self_port;
std::string G_protocol;

std::string G_hardware_specs_filename;

enum save_type G_out_save_type;
uint8_t G_pixel_connection;
bool G_colored;
std::string G_input_name;
std::string G_out_name;
std::string G_out_ext;
Tattribute G_lambda;
uint32_t G_glines, G_gcolumns;
uint32_t G_num_threads;

boundary_tree *G_full_bound_tree;
bool G_full_bound_tree_received = false;
std::mutex G_full_bound_tree_lock;
std::condition_variable G_cv;

scheduler_of_workers<worker *> G_local_workers;

// std::vector<std::thread *> workers_threads;



/* ======================= signatures ================================= */
void verify_args(int argc, char *argv[]);
void read_config(char conf_name[]);
std::unordered_map<std::string, float> read_CPU_SPECS(std::string cpu_sepcs = "CPU_INFO.data");
void registry_new_worker(uint32_t local_id, std::string server_addr, std::unordered_map<std::string, TWorkerAttr> &worker_attr);
void registry_threads(uint32_t num_th, std::string server_addr, std::string self_addr);
bool do_work(vips::VImage *img_in, worker *w);
void loop_worker(vips::VImage *img, std::string server_addr);
void make_worker_threads(uint32_t numth, VImage *in, std::string server);
/* ======================= implementations ================================= */

void verify_args(int argc, char *argv[]){
    std::cout << "argc: " << argc << " argv:" ;
    for(int i=0;i<argc;i++){
        std::cout << argv[i] << " ";
    }
    std::cout << "\n";
    if(argc <= 2){
        std::cout << "usage: " << argv[0] << " input_image config_file\n";
        exit(EX_USAGE);
    }
}

/* Read configuration file to global variables */    
void read_config(char conf_name[]){
    verbose=false;
    print_only_trees=false;
    
    auto configs = parse_config(conf_name);

    if(configs->find("glines") == configs->end() || configs->find("gcolumns") == configs->end()){
        std::cout << "you must specify the the image division on config file:" << conf_name <<"\n";
        std::cout << "example:\n";
        std::cout << "glines=8 #divide image in 8 vertical tiles\n";
        std::cout << "gcolumns=6 #divide image in 6 vertical tiles\n";
        exit(EX_CONFIG);
    }
    G_glines = std::stoi(configs->at("glines"));
    G_gcolumns = std::stoi(configs->at("gcolumns"));

    G_input_name = get_field(configs, "input", "");
    
    auto str_G_lambda = get_field(configs, "lambda", "1");
    G_lambda = std::stod(str_G_lambda);
    
    G_server_ip = get_field(configs, "server_ip", "127.0.0.1");
    
    G_server_port = get_field(configs, "server_port", DEFAULT_PORT);

    G_self_port = get_field(configs, "self_port", DEFAULT_PORT);

    G_protocol = get_field(configs, "protocol", "tcp");

    G_out_name = get_field(configs, "output", "output");
    
    G_out_ext = get_field(configs, "output_ext", "png");

    G_hardware_specs_filename = get_field(configs, "hw_specs", "hardware.yaml");

    auto str_G_num_threads = get_field(configs, "threads", "");
    G_num_threads = std::stoi(str_G_num_threads);
    
    G_pixel_connection = 4;

    auto str_verbose = get_field(configs, "verbose", "false");
    verbose = str_verbose == "true";
    
    auto str_G_colored = get_field(configs, "colored", "false");
    G_colored = str_G_colored == "true";

    auto str_save = get_field(configs, "join_image", "full_image");
    G_out_save_type = FULL_IMAGE;
    if(str_save == "save_split"){
        G_out_save_type = SPLIT_IMAGE;
    }else if(str_save == "no_save"){
        G_out_save_type = NO_SAVE;
    }

    
    auto str_vips_access = get_field(configs, "vips_access", "VIPS_ACCESS_RANDOM");
    try{
        G_vips_access = VipsAccesTypeVector[str_vips_access];
    }catch(...){
        std::cerr << "Access type " << str_vips_access << "not defined or not supported.\nUsing VIPS_ACCESS_RANDOM.\n";
    }
    

    // std::cout << "configurations:\n";
    print_unordered_map(configs);
    // std::cout << "====================\n";
    
    delete configs;
    
}

void worker_read_attributes(worker *w, std::string conf_workers_file){
    auto configs = parse_hw_config(conf_workers_file);
}

void registry_new_worker(uint32_t local_id, std::string server_addr, std::unordered_map<std::string, TWorkerAttr> &worker_attr){

    worker *w = new worker(local_id, server_addr, G_self_ip + " | pid= " +std::to_string(getpid()));
    
    for(auto k_v: worker_attr){
        w->set_attr(k_v.first, k_v.second);
    }
    // std::cout << "registring id: " << local_id << " of total " << num_th << " threads\n";
    // workers_threads.push_back(new std::thread(w->registry_at, server_addr));
    // workers_threads.push_back(std::thread(&worker::registry,w));
    w->registry();
    G_local_workers.insert_worker(w);
    // sleep(1);
}



void registry_threads(uint32_t num_th, std::string server_addr, std::string self_addr){
    // std::cout << "number of threads "<< num_th << "\n";  
    std::cout << "start registration\n";
    std::vector<std::thread> workers_threads;
    auto workers_conf = parse_hw_config(G_hardware_specs_filename);
    size_t wcs = workers_conf->size();
    //workers register phase
    for(uint32_t local_id=0; local_id < num_th; local_id++){
        std::cout << "registrando worker " << local_id<< "\n";
        workers_threads.push_back(std::thread(registry_new_worker, local_id, server_addr, std::ref(workers_conf->at(local_id % wcs))));
    }
    for(size_t i=0; i<workers_threads.size(); i++){
        workers_threads[i].join();
    }
    free_hw_config(workers_conf);
    // read_sequential_file(G_bag_tiles, in, glines, gcolumns);
    // std::cout << "G_bag_tiles.get_num_task:" << G_bag_tiles.get_num_task() << "\n";

}

void request_process_tile(vips::VImage *img_in, message &msg_work, worker *w){
    std::string s_tile = msg_work.content;
    auto tile = hps::from_string<std::pair<uint32_t,uint32_t>>(s_tile);
    // if(tile == GRID_DIMS){
    //     // std::cout << "tile has GRID_DIMS value\n";
    //     return false;
    // }
    std::string msg = "tile received: (" + std::to_string(tile.first) + "," + std::to_string(tile.second) 
                    + ") worker:" + std::to_string(w->get_index()) +"\n";
    // std::cout << msg;
    input_tile_task *t = new input_tile_task(tile);

    t->prepare(img_in, G_glines, G_gcolumns);
    t->read_tile(img_in);

    maxtree_task *mtt = new maxtree_task(t);
    mtt->compute();

    G_maxtrees.insert_task(mtt);

    // std::cout << "maxtree\n" << mtt.mt->to_string() << "\n--------------\n";
    auto init_pair = std::make_pair<uint32_t, uint32_t>(0,1);
    boundary_tree_task btt = boundary_tree_task(mtt, init_pair);
    // std::cout << "boundary tree\n"; 
    // btt.bt->print_tree();

    w->send_btree_task(&btt,MSG_BOUNDARY_TREE);
    std::cout << "sending tree: " << btt.bt->grid_i << ", " << btt.bt->grid_j << ")\n";
    btt.free_tree();
}

void merge_tiles(message &msg_work, worker *w){
    // enum merge_directions merge_dir;
    // enum neighbor_direction nb_direction;
    std::pair<uint32_t, uint32_t> nb_dist;
    std::string s_merge_task = msg_work.content;
    merge_btrees_task mbtt = hps::from_string<merge_btrees_task>(s_merge_task);
    std::string s;
    // std::cout << "merging\n";
    // mbtt.bt1->print_idx();
    // mbtt.bt2->print_idx();
    boundary_tree *merged_tree = mbtt.execute();
    // std::cout << "MERGE DONE\n";
    mbtt.free_trees();
    nb_dist = std::make_pair<uint32_t, uint32_t>(mbtt.distance.first * 2, mbtt.distance.second * 2);
    if(nb_dist.second >= GRID_DIMS.second){
        nb_dist.first = 1;
        nb_dist.second = 0;
    }
    // s= " merge tiles end "+ mbtt.bt1->index_to_string() + " " + mbtt.bt2->index_to_string();
    // s+="merge distance: " + int_pair_to_string(mbtt.distance) + "\n";
    // std::cout << s;
    boundary_tree_task btt = boundary_tree_task(merged_tree, nb_dist);
    std::string _m = "sending merged tree of worker " + std::to_string(w->get_index()) + "\n";
    std::cout << _m;
    w->send_btree_task(&btt, MSG_SEND_MERGED_TREE);

    _m = "worker "+ std::to_string(w->get_index()) +" send merge tree finish\n";
    std::cout << _m;

    // s = "sent: "  + btt.bt->index_to_string() ;
    // s += " {" + mbtt.bt1->index_to_string() + " " + mbtt.bt2->index_to_string() + "}\n";
    // std::cout << s;
    btt.free_tree();
}

void receive_global_boundary_tree(message &m){
    std::unique_lock<std::mutex> lock(G_full_bound_tree_lock);
    if(!G_full_bound_tree_received){
        if(m.content == ""){
            G_cv.wait(lock);
        }
        // std::cout << "receiving global boundary tree...\n";
        if(!G_full_bound_tree_received){
            // std::cout << "content:\n" << m.content << "\n========\n";
            boundary_tree rec_bt = hps::from_string<boundary_tree>(m.content);
            G_full_bound_tree = new boundary_tree(rec_bt);
            G_full_bound_tree_received = true;
        }
        G_cv.notify_all();
        // std::cout << "received\n";
    }
}

void update_filter_and_save(maxtree_task *t){
        std::string output_name = G_out_name +std::to_string(t->mt->grid_i)+"-"+std::to_string(t->mt->grid_j)+"."+G_out_ext;
        t->update_tree(G_full_bound_tree);
        t->filter_tree(G_lambda);
        t->mt->save(output_name);
        
        std:: string sout = "file save:" + output_name + "\n";
        std::cout << sout;
}

bool do_work(vips::VImage *img_in, worker *w){
    maxtree_task *update_task=nullptr;
    // maxtree *m;
    message msg_work = w->request_work();
    
    // std::cout << "request task\n";
    std::string sout;
    bool ret=true;

    std::cout << "received " << NamesMessageType[msg_work.type] << "\n";
    // if(msg_work.type != MSG_NULL){
    //     sout ="===============> type:" + std::to_string(msg_work.type) + " -> " + NamesMessageType[msg_work.type] + " to worker:" + std::to_string(w->get_index()) + "<===============\n";
    //     std::cout << sout;
    // }
    
    if(msg_work.type == MSG_TILE_IDX){
        request_process_tile(img_in, msg_work, w);
    }else if(msg_work.type == MSG_MERGE_BOUNDARY_TREE){
        merge_tiles(msg_work, w);
    }else if(msg_work.type == MSG_UPDATE_BOUNDARY_TREE){
        receive_global_boundary_tree(msg_work);
        sout = "tasks waiting for update: " + std::to_string(G_maxtrees.size()) + "\n";
        std::cout << sout;
        if(G_maxtrees.get_task(update_task)){
            update_filter_and_save(update_task);
        }
    }else if(msg_work.type == MSG_COMMAND && msg_work.content == "END"){
        sout = "finishing work " + std::to_string(w->get_index()) + "\n";
        std::cout << sout;
        ret = false;
    }
    
    // if(msg_work.type != MSG_NULL){
    //     std::string sout ="===============> message of type:" + std::to_string(msg_work.type) + " -> " + NamesMessageType[msg_work.type] + " finished at worker:" + std::to_string(w->get_index()) + "<===============\n";
    //     std::cout << sout;
    // }
    return ret;
}

void loop_worker(vips::VImage *img, std::string server_addr){
    worker *w=G_local_workers.get_worker();
    
    w->connect();
    while(do_work(img,  w)); // std::cout << it++ << "\n";

    maxtree_task *update_task=nullptr;
    
    while(G_maxtrees.get_task(update_task)){
        update_filter_and_save(update_task);
    }
    std::string sout = "worker " + std::to_string(w->get_index()) + " finished \n";
    // std::cout << sout;
    w->disconnect();/*  */
    
}


void make_worker_threads(uint32_t numth, VImage *in, std::string server){
    std::vector<std::thread> workers_threads;
    for(uint32_t i=0; i<numth; i++){
        workers_threads.push_back(std::thread(loop_worker, in, server));
    }
    for(size_t i=0; i<workers_threads.size(); i++){
        workers_threads[i].join();
    }
}

int main(int argc, char *argv[]){
    vips::VImage *in;
    enum save_type out_save_type;
    maxtree *t;
    input_tile_task *tile;

    if(argc < 2){
        std::cout << "usage:" << argv[0] << "<configuration file> [input image] [output prefix name]\n"
                  << "ps: input image must have it extension\n"
                  << "    the input image and output prefix can be specifyed at configuration file"
                  << "    when these information were passed in command line, the configuration file values will be ignored\n";
        exit(EX_USAGE); 
    }

    read_config(argv[1]);

    if(argc >= 3){
        G_input_name = argv[2];
    }else if(G_input_name == ""){
        std::cout << "input file not defined, please, define it in config file or in command line.\n";
        exit(EX_NOINPUT);
    }

    if(argc >= 4){
        G_out_name = argv[3];
    }

    G_self_ip = "localhost";
    GRID_DIMS = std::make_pair(G_glines,G_gcolumns);

    if (VIPS_INIT(argv[0])) { 
        vips_error_exit (NULL);
    } 
    // SEE VIPS_CONCURRENCY
    // check https://github.com/libvips/libvips/discussions/4063 for improvements on read
    in = new vips::VImage(
            vips::VImage::new_from_file(G_input_name.c_str(),
            VImage::option()->set("access",  G_vips_access)
        )
    );

    std::string server_addr = G_protocol + "://" + G_server_ip + ":" + G_server_port;
    std::string self_addr = G_protocol + "://" + G_self_ip + ":" + G_self_port;

    registry_threads(G_num_threads, server_addr, self_addr);
    // calc_tile_boundary_tree(G_num_threads, server_addr, self_addr);
    // calc_tile_boundary_tree(in, server_addr);
    // test_send_bound_tree(in, server_addr);
    // std::cout << "threads registered\n";
    has_tasks=true;
    
    make_worker_threads(G_num_threads, in, server_addr);

    // apos registrar as threads, o fluxo será o seguinte:
    // pedir tile, calcular maxtree e boundary tree responder boundary tree

    // com todas boundary trees calculadas, agora o fluxo será
    // pedir tarefa de merge, realizar merge;


    for(size_t i=0; i < G_local_workers.size(); i++){
        worker *w = G_local_workers.at(i);
        // std::cout << "delete worker local: " <<  i << " registered as " << w->get_index() << " at manager\n";
        // delete w;
    }

    
    
    
}