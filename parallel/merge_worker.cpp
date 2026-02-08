#include <vips/vips8>
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

bool print_only_trees;
bool verbose;

extern std::pair<uint32_t, uint32_t> GRID_DIMS;

bag_of_tasks<input_tile_task *> bag_tiles;
bag_of_tasks<maxtree_task *> maxtrees;
bag_of_tasks<boundary_tree_task *> boundary_trees;
bag_of_tasks<maxtree_task *> maxtree_dest;


std::string server_ip, self_ip;
std::string server_port, self_port;
std::string protocol;

enum save_type out_save_type;

uint8_t pixel_connection;
bool colored;

std::string input_name;
std::string out_name;
std::string out_ext;

Tattribute lambda;
uint32_t glines;
uint32_t gcolumns;
uint32_t num_threads;

scheduler_of_workers<worker *> local_workers;
std::vector<std::thread *> workers_threads;

/* ======================= signatures ================================= */
template<typename T>
void wait_empty(bag_of_tasks<T> &b, uint64_t num_th);

void verify_args(int argc, char *argv[]);
void read_config(char conf_name[]);
void read_sequential_file(bag_of_tasks<input_tile_task*> &bag, vips::VImage *in, uint32_t glines, uint32_t gcolumns);
void worker_maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, bag_of_tasks<maxtree_task *> &max_trees);



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

template<typename T>
void wait_empty(bag_of_tasks<T> &b, uint64_t num_th){
    if(verbose) std::cout << "wait end\n";
    uint64_t x=0;
    while(true){
        // if(verbose) std::cout << "wait empty " << x++ << "\n";
        b.wait_empty();
        if(b.is_running() && b.empty() && b.num_waiting() == num_th){
            b.notify_end();
            // if(verbose) std::cout << "end notified\n";
            break;
        }
    }
}


std::string get_field(std::unordered_map<std::string, std::string> *conf, std::string field, std::string dft){
    if(conf->find(field) != conf->end()){
        return conf->at(field);
    }
    return dft;
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
    glines = std::stoi(configs->at("glines"));
    gcolumns = std::stoi(configs->at("gcolumns"));

    input_name = get_field(configs, "input", "");
    
    auto str_lambda = get_field(configs, "lambda", "1");
    lambda = std::stod(str_lambda);
    
    server_ip = get_field(configs, "server_ip", "127.0.0.1");
    
    server_port = get_field(configs, "server_port", DEFAULT_PORT);

    self_port = get_field(configs, "self_port", DEFAULT_PORT);

    protocol = get_field(configs, "protocol", "tcp");

    out_name = get_field(configs, "output", "output");
    
    out_ext = get_field(configs, "output_ext", "png");
    
    auto str_num_threads = get_field(configs, "threads", "");
    num_threads = std::stoi(str_num_threads);
    
    pixel_connection = 4;

    auto str_verbose = get_field(configs, "verbose", "false");
    verbose = str_verbose == "true";
    
    auto str_colored = get_field(configs, "colored", "false");
    colored = str_colored == "true";

    auto str_save = get_field(configs, "join_image", "full_image");
    out_save_type = FULL_IMAGE;
    if(str_save == "save_split"){
        out_save_type = SPLIT_IMAGE;
    }else if(str_save == "no_save"){
        out_save_type = NO_SAVE;
    }

    std::cout << "configurations:\n";
    print_unordered_map(configs);
    std::cout << "====================\n";
    
}

void read_sequential_file(bag_of_tasks<input_tile_task*> &bag, vips::VImage *in, uint32_t glines, uint32_t gcolumns){
    input_tile_task *nt;
    for(uint32_t i=0; i<glines; i++){
        for(uint32_t j=0; j<gcolumns; j++){
            nt = new input_tile_task(i,j);
            nt->prepare(in,glines,gcolumns);
            nt->read_tile(in);
            bag.insert_task(nt);
        }
    }

}

void registry(worker *w, std::string server_addr, std::string self_addr){
    zmq::context_t context(1);
    zmq::socket_t conect_manager(context, zmq::socket_type::req);
    conect_manager.connect(server_addr);
    std::string msg_content = hps::to_string(*w);
    // std::cout << "msg content:" << msg_content << "\n";
    message requirement(msg_content, msg_content.size(), MSG_REGISTRY, self_addr);
    std::string s_msg = hps::to_string(requirement);
    // std::cout << "sending: " << s_msg << "\n";
    zmq::message_t message_0mq(s_msg.size());
    memcpy(message_0mq.data(), s_msg.data(), s_msg.size());
    std::string global_id="";

    conect_manager.send(message_0mq, zmq::send_flags::none);
    zmq::message_t reply_0mq;
    auto resp_val = conect_manager.recv(reply_0mq, zmq::recv_flags::none);
    TWorkerIdx reply; // = hps::from_string<TWorkerIdx>(reply_0mq);
    memcpy(&reply, reply_0mq.data(), sizeof(TWorkerIdx));
    std::cout << "reply: "<< reply << "\n";
    w->update_index(reply);
}




void registry_threads(uint32_t num_th, std::string server_addr, std::string self_addr){
    std::cout << "number of threads "<< num_th << "\n";  
    std::cout << "start registration\n";
    //workers register phase
    for(uint16_t local_id=0; local_id < num_th; local_id++){
        worker *w = new worker(local_id, nullptr);
        w->set_attr("MHZ", 4000.041);
        w->set_attr("L3", 16.0+local_id);
        
        
        std::cout << "registring id: " << local_id << " of total " << num_th << " threads\n";
        workers_threads.push_back(new std::thread(registry, w, server_addr, self_addr));
        local_workers.insert_worker(w);
        
    }
    // read_sequential_file(bag_tiles, in, glines, gcolumns);
    std::cout << "bag_tiles.get_num_task:" << bag_tiles.get_num_task() << "\n";

    /* bag_tiles.start();
    for(uint32_t i=0; i<num_th; i++){
        threads_g1.push_back(new std::thread(worker_maxtree_calc, std::ref(bag_tiles), std::ref(maxtree_tiles_pre_btree)));
    }*/
}


void test(vips::VImage *in, std::string server_addr){
    input_tile_task *task = new input_tile_task(1,1);
    task->prepare(in,3,3);
    task->read_tile(in);

    maxtree_task *mttask = new maxtree_task(task);

    maxtree *t = mttask->mt;
    
    std::cout << "maxtree to string: \n" << t->to_string() << "\n";
    boundary_tree *bt = t->get_boundary_tree();
    // std::cout << "lroot string:" << bt->lroot_to_string(BOUNDARY_GVAL, " - ") << "\n=======================\n";
    // bt->print_tree();
    

    std::string msg_content = hps::to_string(*bt);
    message m = message(msg_content, msg_content.size(), MSG_BOUNDARY_TREE);

    zmq::context_t context(1);
    
    zmq::socket_t sender(context, zmq::socket_type::req);

    sender.connect(server_addr);
    std::string s_msg = hps::to_string(m);

    std::cout << "sending: -->" << s_msg << "<--\n";
    std::cout << "sending: -->";
    for(char c: s_msg){
        std::cout << " " << (int) c;
    }
    std::cout << " <--\n";
    std::cout << "sending tree\n";
    zmq::message_t *message_0mq = new zmq::message_t(s_msg.size());
    memcpy(message_0mq->data(), s_msg.data(), s_msg.size());
    sender.send(*message_0mq, zmq::send_flags::none);

    std::cout << "tree sent\n";
    delete message_0mq;

   
}

int main(int argc, char *argv[]){
    vips::VImage *in;
    std::string out_name, out_ext;// server_ip, self_ip, server_port, self_port, protocol;
    
    uint32_t glines, gcolumns;
    uint8_t pixel_connection;

    bool colored;
    enum save_type out_save_type;
    Tattribute lambda=2;
    maxtree *t;
    input_tile_task *tile;

    
    bag_of_tasks<input_tile_task*> bag_tiles;
    bag_of_tasks<maxtree_task*> maxtree_tiles_pre_btree, maxtree_tiles, updated_trees;
    bag_of_tasks<boundary_tree_task *> boundary_bag, boundary_bag_aux;
    bag_of_tasks<merge_btrees_task *> merge_bag;

    std::vector<std::thread*> threads_g1, threads_g2, threads_g3, threads_g4;
    if(argc < 2){
        std::cout << "usage:" << argv[0] << "<configuration file> [input image] [output prefix name]\n"
                  << "ps: input image must have it extension\n"
                  << "    the input image and output prefix can be specifyed at configuration file"
                  << "    when these information were passed in command line, the configuration file values will be ignored\n";
        exit(EX_USAGE); 
    }

    // verify_args(argc, argv);
    // read_config(argv[1], input_name, out_name, out_ext, glines, gcolumns, lambda, 
    //             pixel_connection, colored, num_th, out_save_type, server_ip, server_port,
    //             self_port, protocol);
    read_config(argv[1]);

    if(argc >= 3){
        input_name = argv[2];
    }else if(input_name == ""){
        std::cout << "input file not defined, please, define it in config file or in command line.\n";
        exit(EX_NOINPUT);
    }

    if(argc >= 4){
        out_name = argv[3];
    }

    self_ip = "localhost";

    GRID_DIMS = std::make_pair(glines,gcolumns);

    if (VIPS_INIT(argv[0])) { 
        vips_error_exit (NULL);
    } 
    // SEE VIPS_CONCURRENCY
    // check https://github.com/libvips/libvips/discussions/4063 for improvements on read
    in = new vips::VImage(
            vips::VImage::new_from_file(input_name.c_str(),
            VImage::option()->set ("access",  VIPS_ACCESS_SEQUENTIAL)
        )
    );

    std::string server_addr = protocol + "://" + server_ip + ":" + server_port;
    std::string self_addr = protocol + "://" + self_ip + ":" + self_port;

    registry_threads(num_threads, server_addr, self_addr);
    
    test(in, server_addr);
    
    // apos registrar as threads, o fluxo será o seguinte:
    // pedir tile, calcular maxtree e boundary tree responder boundary tree

    // com todas boundary trees calculadas, agora o fluxo será
    // pedir tarefa de merge, realizar merge;
    



    for(size_t i=0; i<workers_threads.size(); i++){
        workers_threads[i]->join();
    }

    for(size_t i=0; i<workers_threads.size(); i++){
        delete workers_threads[i];
    }
    for(size_t i=0; i < local_workers.size(); i++){
        worker *w = local_workers.at(i);
        std::cout << "worker local: " <<  i << " registered as " << w->get_index() << " at manager\n";
    }
    
    
}