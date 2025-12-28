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

using namespace vips;
bool print_only_trees;
bool verbose;

extern std::pair<uint32_t, uint32_t> GRID_DIMS;


/* ======================= signatures ================================= */
template<typename T>
void wait_empty(bag_of_tasks<T> &b, uint64_t num_th);

void verify_args(int argc, char *argv[]);
void read_config(char conf_name[], std::string &input_name, std::string &out_name, std::string &out_ext,
                 uint32_t &glines, uint32_t &gcolumns, Tattribute &lambda,
                 uint8_t &pixel_connection, bool &colored, uint32_t &num_threads,
                 std::string &server_ip, std::string server_port, std::string &self_port);
void read_sequential_file(bag_of_tasks<input_tile_task*> &bag, vips::VImage *in, uint32_t glines, uint32_t gcolumns);
bool inside_rectangle(std::pair<uint32_t, uint32_t> c, std::pair<uint32_t, uint32_t> r);
std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t);

void worker_maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, bag_of_tasks<maxtree_task *> &max_trees);
void worker_get_boundary_tree(bag_of_tasks<maxtree_task *> &maxtrees, bag_of_tasks<boundary_tree_task *> &boundary_trees, bag_of_tasks<maxtree_task *> &maxtree_dest);
void worker_search_pair(bag_of_tasks<boundary_tree_task *> &btrees_bag, bag_of_tasks<merge_btrees_task *> &merge_bag);
void worker_merge_local(bag_of_tasks<merge_btrees_task *> &merge_bag, bag_of_tasks<boundary_tree_task *> &btrees_bag);
void worker_update(bag_of_tasks<maxtree_task *> &src, bag_of_tasks<maxtree_task *> &dest, boundary_tree *global_bt);

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

void read_config(char conf_name[], std::string &input_name, std::string &out_name, std::string &out_ext,
                 uint32_t &glines, uint32_t &gcolumns, Tattribute &lambda, uint8_t &pixel_connection, 
                 bool &colored, uint32_t &num_threads, enum save_type &out_save_type,
                 std::string &server_ip, std::string &server_port, std::string &self_port, std::string &protocol){
    /*
        Reading configuration file
    */
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
/* check if pair of coordinates c is inside the rectangle r (starting at origin (0,0) and 
    ending in position (r.first-1, r.second-1) */
bool inside_rectangle(std::pair<uint32_t, uint32_t> c, std::pair<uint32_t, uint32_t> r){
    if(c.first >= r.first || c.first < 0 || c.second >= r.second || c.second < 0){
        return false;
    }
    return true;
}

//get a task from bag and compute maxtree of the tile of this task
void worker_maxtree_calc(bag_of_tasks<input_tile_task *> &bag_tiles, bag_of_tasks<maxtree_task *> &max_trees){
    bool got_task;
    input_tile_task *t;
    maxtree_task *mt;
    while(bag_tiles.is_running()){
        if(verbose){
            std::ostringstream os("");
            os << "thread " << std::this_thread::get_id() << " trying to get task\n";
            std::string s = os.str();
            std::cout << s;
        }
        got_task=bag_tiles.get_task(t);
        if(got_task){
            if(verbose){
                std::ostringstream os("");
                os << "worker " <<  std::this_thread::get_id() << " got task " << t->i << ", " << t->j << " to calculate maxtree\n";
                std::string s = os.str();
                std::cout << s;
            }
            mt = new maxtree_task(t);
            max_trees.insert_task(mt);
        }
        if(verbose){
            std::ostringstream os("");
            os << "thread " << std::this_thread::get_id() << " couldnt get task\n";
            std::string s = os.str();
            std::cout << s; 
        }
    }
}

void worker_get_boundary_tree(bag_of_tasks<maxtree_task *> &maxtrees,
                              bag_of_tasks<boundary_tree_task *> &boundary_trees, 
                              bag_of_tasks<maxtree_task *>  &maxtree_dest){
    bool got_task;
    maxtree_task *mtt;
    boundary_tree_task *btt;
    bag_of_tasks<maxtree_task *> maxtree_aux;
    while(maxtrees.is_running()){
        got_task = maxtrees.get_task(mtt);
        if(got_task){
            // std::string stmt = std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) +"\n";
            // stmt += mtt->mt->to_string(GLOBAL_IDX,false) + "\n";
            // std::cout << stmt;
            btt = new boundary_tree_task(mtt, std::make_pair<uint32_t,uint32_t>(0,1));
            boundary_trees.insert_task(btt);
            maxtree_dest.insert_task(mtt);
        }

    }

}
std::pair<uint32_t, uint32_t> get_task_index(boundary_tree_task *t){
    return t->index;
}


void worker_search_pair(bag_of_tasks<boundary_tree_task *> &btrees_bag, bag_of_tasks<merge_btrees_task *> &merge_bag){
    boundary_tree_task *btt, *n, *aux;
    std::pair<uint32_t, uint32_t> idx_nb;
    enum neighbor_direction nb_direction;
    enum merge_directions merge_dir;
    uint32_t new_distance;
    bool change_dir;
    std::string s;
    while(btrees_bag.is_running() || !btrees_bag.get_num_task() > 1){

        bool got = btrees_bag.get_task(btt);
        if(got){
            if(btt->nb_distance.first == 0){
                merge_dir = MERGE_VERTICAL_BORDER;
            }else if(btt->nb_distance.second == 0){
                merge_dir = MERGE_HORIZONTAL_BORDER;
            }else{
                std::cerr << "merge distance invalid: " << int_pair_to_string(btt->nb_distance) << "\n";
                exit(EXIT_FAILURE);
            }
            // s = "got tree: " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + 
            //     " with distance: "+ int_pair_to_string(btt->nb_distance) +"\n";
            // std::cout << s;
            if(merge_dir == MERGE_VERTICAL_BORDER){
                if(btt->bt->grid_j % int_pow(2,btt->nb_distance.second) == 0){
                    nb_direction = NB_AT_RIGHT;
                }else{
                    nb_direction = NB_AT_LEFT;
                }
            }else if(merge_dir == MERGE_HORIZONTAL_BORDER){
                if(btt->bt->grid_i % int_pow(2,btt->nb_distance.first) == 0){
                    nb_direction = NB_AT_BOTTOM;
                }else{
                    nb_direction = NB_AT_TOP;
                }
            }
            idx_nb = btt->neighbor_idx(nb_direction);
            if(inside_rectangle(idx_nb, GRID_DIMS) && inside_rectangle(btt->index, GRID_DIMS)){
                try{
                    auto got_n = btrees_bag.get_task_by_field<std::pair<uint32_t,uint32_t>>(n, idx_nb, get_task_index);
                    
                    if(!got_n){
                        btrees_bag.insert_task(btt);
                        // std::string s = "no neighbor" + std::to_string(btt->index.first) + "," + std::to_string(btt->index.second) + "\n";
                        // std::cout << s;
                    }else if (n->nb_distance.first == btt->nb_distance.first && n->nb_distance.second == btt->nb_distance.second){

                        if((merge_dir == MERGE_VERTICAL_BORDER) && (btt->bt->grid_j > n->bt->grid_j) ||
                           (merge_dir == MERGE_HORIZONTAL_BORDER) && (btt->bt->grid_i > n->bt->grid_i)){
                            if(verbose){
                                std::string s = "swap: " + int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + "\n";
                                std::cout << s;
                            }
                            aux = btt;
                            btt = n;
                            n = aux;
                        }
                        if(merge_dir == MERGE_HORIZONTAL_BORDER&& btt->bt->border_elements->at(BOTTOM_BORDER)->size() != n->bt->border_elements->at(TOP_BORDER)->size()){
                            std::string s = int_pair_to_string(btt->index) + " and " + int_pair_to_string(n->index) + " borders differs in size\n";
                            s += "distances:" + int_pair_to_string(btt->nb_distance) + " and " + int_pair_to_string(n->nb_distance) + "\n";
                            std::cout << s;
                        }

                        // s = "creating task with btt " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + "   ";
                        // s += "n: "  + std::to_string(n->bt->grid_i) + "," + std::to_string(n->bt->grid_j) + "distance ";
                        // s += int_pair_to_string(btt->nb_distance) +"\n";
                        // // std::cout << s;
                        auto new_merge_task = new merge_btrees_task(btt->bt, n->bt, merge_dir, btt->nb_distance);
                        merge_bag.insert_task(new_merge_task);
                    }else{
                        if(verbose){
                            std::string s = "invalid distance:" + int_pair_to_string(btt->index) + " ->" + int_pair_to_string(btt->nb_distance) + "\n";
                            s+="invalid distance:" + int_pair_to_string(n->index) + " ->" + int_pair_to_string(n->nb_distance) + "\n=======\n";
                            std::cout << s;
                        }
                        btrees_bag.insert_task(n);
                        btrees_bag.insert_task(btt);
                    }
                }
                catch(std::runtime_error &e){
                    std::cerr << e.what();
                    // s = "pair of " + std::to_string(btt->bt->grid_i) + "," + std::to_string(btt->bt->grid_j) + " not found ";
                    // s += std::to_string(idx_nb.first) + "," + std::to_string(idx_nb.second) + "\n";
                    // std::cout << s;
                    btrees_bag.insert_task(btt);
                }catch(std::out_of_range &r){
                    std::cerr << "try to access an out of range element\n";
                }
            }else if (inside_rectangle(btt->index, GRID_DIMS)){ // the neighbor of btt is not inside the grid (it does not exist, so go to next merge)
                if(btt->nb_distance.first == 0){
                    if(btt->nb_distance.second < GRID_DIMS.second){ //this tile doesn't need to merge, just try to found a neighbor further than the actual
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " line distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.second *= 2;
                    }else{ // there is no more neighbor on this line to merge, so this line must be merged with the other lines
                        
                        btt->nb_distance.first = 1; 
                        btt->nb_distance.second = 0;
                    }
                }else if(btt->nb_distance.second == 0){
                    if(btt->nb_distance.first < GRID_DIMS.first){
                        if(verbose){
                            std::string s = int_pair_to_string(btt->index) + " column distance * 2 = "+ int_pair_to_string(btt->nb_distance) + "\n";
                            std::cout << s;
                        }
                        btt->nb_distance.first *= 2;
                    }
                }
                btrees_bag.insert_task(btt);   
            }
        }
    }
    if(verbose) std::cout << "end worker search pair\n";
}
void worker_merge_local(bag_of_tasks<merge_btrees_task *> &merge_bag, bag_of_tasks<boundary_tree_task *> &btrees_bag){
    merge_btrees_task *mbt;
    boundary_tree_task *btt;
    boundary_tree *nbt;
    std::pair<uint32_t, uint32_t> dist;
    std::string s;
    while(merge_bag.is_running() || !merge_bag.empty()){

        bool got_mt = merge_bag.get_task(mbt);
        if(got_mt){
            if(verbose){
                s = "-------------------TREE 1------------------\n";
                // mbt->bt1->print_tree();
                s+=mbt->bt1->border_to_string();
                s+=mbt->bt1->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                s = "-------------------TREE 2------------------\n";
                // mbt->bt2->print_tree();
                s+=mbt->bt2->border_to_string();
                s+=mbt->bt2->lroot_to_string();
                s+="\n-----------------------------------------------\n";
                // std::cout << s;
                std::cout << s;
                s = "task will merge: " + std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + " \n";
                std::cout << s;
            }
            nbt = mbt->execute();
            
            dist.second = (mbt->bt2->grid_j - mbt->bt1->grid_j) * 2;
            dist.first = (mbt->bt2->grid_i - mbt->bt1->grid_i) * 2;
            if(dist.second >= GRID_DIMS.second){
                dist.first = 1;
                dist.second = 0;
            }else if(dist.second == 0 && dist.first >= GRID_DIMS.first){
                if(verbose){
                    std::string s = "ending  merge --- dist:" + int_pair_to_string(dist) + " " ;
                    s += std::to_string(mbt->bt1->grid_i) + ", " + std::to_string(mbt->bt1->grid_j) ;
                    s += " with " + std::to_string(mbt->bt2->grid_i) + ", " + std::to_string(mbt->bt2->grid_j) + "\n";
                    std::cout << s;
                }
                merge_bag.notify_end();
                btrees_bag.notify_end();
            }

            if(verbose) nbt->print_tree();
            btt = new boundary_tree_task(nbt, dist);
            if(verbose){
                s += " task inserted with index:" + std::to_string(btt->bt->grid_i) + ", " + std::to_string(btt->bt->grid_j);
                s += " and distance " + int_pair_to_string(btt->nb_distance) + "\n";
                std::cout << s;
            }
            btrees_bag.insert_task(btt);
        }
    }
    if(verbose)std::cout << "end worker local merge\n";
}


void worker_update_filter(bag_of_tasks<maxtree_task *> &src, bag_of_tasks<maxtree_task *> &dest, boundary_tree *global_bt, Tattribute lambda){
    bool got_task;
    maxtree_task *mtt;
    std::string s;
    if(verbose) std::cout << "worker update\n";
    while(src.is_running()){
        got_task = src.get_task(mtt);
        // s = "updating (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") \n";
        // std::cout << s;
        if(got_task){
            mtt->mt->update_from_boundary_tree(global_bt);
            dest.insert_task(mtt);
            mtt->mt->filter(lambda, global_bt);
        }
        if(verbose) {
            s = "task of grid (" + std::to_string(mtt->mt->grid_i) + "," + std::to_string(mtt->mt->grid_j) + ") update\n";
            std::cout << s;
        }
    }
}

int main(int argc, char *argv[]){
    vips::VImage *in;
    std::string out_name, out_ext, input_name, server_ip, self_ip, server_port, self_port, protocol;
    
    uint32_t glines, gcolumns, num_th;
    uint8_t pixel_connection;

    bool colored;
    enum save_type out_save_type;
    Tattribute lambda=2;
    maxtree *t;
    input_tile_task *tile;
    std::vector<worker *> local_workers;
    
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
    read_config(argv[1], input_name, out_name, out_ext, glines, gcolumns, lambda, 
                pixel_connection, colored, num_th, out_save_type, server_ip, server_port,
                self_port, protocol);

    if(argc >= 3){
        input_name = argv[2];
    }else if(input_name == ""){
        std::cout << "input file not defined, please, define it in config file or in command line.";
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


    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);
    std::string server_addr = protocol + "://" + server_ip + ":" + server_port;
    std::string self_addr = protocol + "://" + self_ip + ":" + self_port;
    socket.connect(server_addr);

    for(uint16_t id_worker=0; id_worker < num_th; id_worker++){
        worker *w = new worker(id_worker, nullptr);
        w->set_attr("MHZ", 4000.041);
        w->set_attr("L3", 16.0*1024.0*1024.0);
        
        local_workers.push_back(w);
        std::string msg_content = hps::to_string(*w);
        message msg(MSG_REGISTRY, msg_content, msg_content.size());
        std::string s_msg = hps::to_string(msg);
        zmq::message_t message_0mq(s_msg.size());
        memcpy(message_0mq.data(), s_msg.data(), s_msg.size());
        socket.send(message_0mq, zmq::send_flags::none);
    }

    std::cout << "number of threads "<< num_th << "\n";  
    std::cout << "start\n";
    //read_sequential_file(bag_tiles, in, glines, gcolumns);
    std::cout << "bag_tiles.get_num_task:" << bag_tiles.get_num_task() << "\n";
    // bag_tiles.start();



/*     bag_tiles.start();
    for(uint32_t i=0; i<num_th; i++){
        threads_g1.push_back(new std::thread(worker_maxtree_calc, std::ref(bag_tiles), std::ref(maxtree_tiles_pre_btree)));
    }
    maxtree_task *mtt;
    wait_empty<input_tile_task *>(bag_tiles, num_th);
    
    for(uint32_t i=0; i<num_th; i++){
        threads_g1[i]->join();
        delete threads_g1[i];
    }
    threads_g1.erase(threads_g1.begin(),threads_g1.end());

    std::cout << "max_trees_tiles.get_num_task:" << maxtree_tiles_pre_btree.get_num_task() << "\n";
    
    maxtree_tiles_pre_btree.start();
    for(uint32_t i=0; i<num_th;i++){
       threads_g1.push_back(new std::thread( worker_get_boundary_tree, std::ref(maxtree_tiles_pre_btree), std::ref(boundary_bag), std::ref(maxtree_tiles) ));
    }
    wait_empty<maxtree_task *>(maxtree_tiles_pre_btree, num_th);

    for(uint32_t i=0; i<num_th; i++){
        threads_g1[i]->join();
        delete threads_g1[i];
        
    }
    threads_g1.erase(threads_g1.begin(),threads_g1.end());
    
    //ver possibilidade de transformar as bags em filas hierarquicas
    //task to get pairs of boundary trees.

    boundary_bag.start();
    merge_bag.start();

    for(uint32_t i=0; i<num_th;i++){
        threads_g1.push_back(new std::thread(worker_search_pair, std::ref(boundary_bag), std::ref(merge_bag) ));
        // threads_g3.push_back(new std::thread(worker_search_pair, &boundary_bag, &merge_bag ));
    }
    
    for(uint32_t i=0; i<num_th;i++){
        threads_g2.push_back(new std::thread(worker_merge_local, std::ref(merge_bag), std::ref(boundary_bag) ));
        // threads_g4.push_back(new std::thread(worker_merge_local, &merge_bag, &boundary_bag ));
    }

    for(uint32_t i=0; i<num_th; i++){
        threads_g1[i]->join();
        delete threads_g1[i];
    } 
    threads_g1.erase(threads_g1.begin(),threads_g1.end());

    for(uint32_t i=0; i<num_th; i++){
        threads_g2[i]->join();
        delete threads_g2[i];
    } 
    threads_g2.erase(threads_g2.begin(),threads_g2.end());
    std::cout << "merge done\n";
    boundary_tree_task *btree_final_task;
    boundary_bag.get_task(btree_final_task);
    // std::cout << btree_final_task->bt->grid_i << "," << btree_final_task->bt->grid_j << "\n";
    
    maxtree_tiles.start();
    for(uint32_t i = 0; i < num_th; i++){
        
        threads_g3.push_back(new std::thread(worker_update_filter, std::ref(maxtree_tiles), std::ref(updated_trees),  btree_final_task->bt, lambda));
    }
    // std::cout << "waiting end of update\n";
    wait_empty<maxtree_task*>(maxtree_tiles, num_th);

    for(uint32_t i=0; i < num_th; i++){
        threads_g3[i]->join();
        delete threads_g3[i];
    }
     std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>> update done <<<<<<<<<<<<<<<<<<<<<<<<<<<\n";

    // vips::VImage end_img;
    // end_img.new_from_memory_copy(in->data(), sizof(Tpixel_value), in->width(), in->height(), 1,  VIPS_FORMAT_UCHAR);
    
    // std::vector<Tpixel_value> data(in->width()*in->height(), 0);
    // input_tile_task aux;
    updated_trees.start();
    maxtree *final_image = new maxtree(in->height(),in->width(),0,0);
    final_image->get_data()->resize(in->height()*in->width());
    while(!updated_trees.empty()){
        bool got = updated_trees.get_task(mtt);
        if(got){
            // std::cout << mtt->mt->grid_i << "," << mtt->mt->grid_j << "\n";
            // mtt->mt->filter(lambda, btree_final_task->bt);
            std::string name = out_name + "_" + std::to_string(mtt->mt->grid_i) + "-" + std::to_string(mtt->mt->grid_j) + "." + out_ext;
            if(out_save_type == SPLIT_IMAGE){
                mtt->mt->save(name);
            }
            if(out_save_type == FULL_IMAGE){
                for(int n = 0; n < mtt->mt->get_size(); n++){
                    maxtree_node *pix = mtt->mt->at_pos(n);
                    final_image->set_pixel(pix, pix->global_idx);
                }
            }
        }
    }
    if(out_save_type == FULL_IMAGE){
        final_image->save(out_name+"."+out_ext);
    } */
}