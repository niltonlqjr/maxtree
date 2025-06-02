#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>
#include <sysexits.h>


#include "maxtree.hpp"
#include "maxtree_node.hpp"
#include "utils.hpp"


#define INQUEUE -2

using namespace vips;



void print_pq(std::priority_queue<maxtree_node*, std::vector<maxtree_node*> ,cmp_maxtree_nodes> pq){
    std::cout <<"===========QUEUE=============\n";
    while(!pq.empty()){
        auto e=pq.top();
        std::cout<<*e<<" ";
        //pq.remove(e);
        pq.pop();
    }
    std::cout<<"\n";
    std::cout <<"============================\n";
}

int main(int argc, char *argv[]){
    vips::VImage *in;
    maxtree *t;
    std::cout << "argc: " << argc << " argv:" ;
    for(int i=0;i<argc;i++){
        std::cout << argv[i] << " ";
    }
    std::cout << "\n";

    

    if(argc <= 2){
        std::cout << "usage: " << argv[0] << " input_image config_file\n";
        exit(EX_USAGE);
    }

    if (VIPS_INIT (argv[0])) {
        vips_error_exit (NULL);
    }

    bool verbose=false;
    in = new vips::VImage(vips::VImage::new_from_file(argv[1],NULL));


    auto configs = parse_config(argv[2]);

    if (configs->find("verbose") != configs->end()){
        if(configs->at("verbose") == "true"){
            verbose=true;
        }
    }

    if(configs->find("glines") == configs->end() || configs->find("gcolumns") == configs->end()){
        std::cout << "you must specify the the image division on config file:" << argv[2] <<"\n";
        std::cout << "example:\n";
        std::cout << "glines=8 #divide image in 8 vertical tiles\n";
        std::cout << "gcolumns=6 #divide image in 6 vertical tiles\n";
        exit(EX_CONFIG);
    }

    uint32_t glines = std::stoi(configs->at("glines"));
    uint32_t gcolumns = std::stoi(configs->at("gcolumns"));

    std::cout << "configurations";
    print_unordered_map(configs);
    std::cout << "====================\n";
    std::cout << "start\n";
    
    int h,w;
    h=in->height();
    w=in->width();
    

    uint32_t h_trunc = h/glines;
    uint32_t w_trunc = w/gcolumns;

    uint32_t num_h_ceil = h%glines;
    uint32_t num_w_ceil = w%gcolumns;

    std::vector<maxtree *> tiles;

    for(uint32_t i=0; i<glines; i++){
        uint32_t tile_lines = i < num_h_ceil ? h_trunc : h_trunc+1;
        for(uint32_t j=0; j<gcolumns; j++){
            uint32_t tiles_columns = j < num_w_ceil ? w_trunc : w_trunc+1;
            tiles.push_back(new maxtree(tile_lines, tiles_columns));
            //create region of image and fill tile from region
        }
    }

    t = new maxtree(h,w);

    std::cout << "full image:" << t->h << ", " << t->w << "\n";

    for(int i=0; i < tiles.size(); i++){
        std::cout << "tile:" << i << " " << tiles.at(i)->h << ", " <<  tiles.at(i)->w << "\n";
    }
    
    vips::VImage cp = in->copy_memory();
   
    t->fill_from_VImage(cp,verbose);

    t->compute_sequential_iterative();
    if(verbose){
        std::cout<<"__________________GVAL________________\n";
        std::cout << t->to_string(GVAL,5);
        std::cout<<"__________________PARENT________________\n";
        std::cout << t->to_string();
    }
    
    
    return 0;
}
