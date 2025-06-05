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
    in = new vips::VImage(vips::VImage::new_from_file(argv[1],
        VImage::option ()->set ("access", VIPS_ACCESS_SEQUENTIAL)));


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

    std::cout << "configurations:\n";
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

    uint32_t i,j,x = 0;
    uint32_t noborder_rt=0, noborder_rl, lines_inc, columns_inc; // original tiles (without borders) size variables
    uint32_t reg_top, reg_left, tile_lines, tile_columns; // tiles used by algorithm (with border) size variables
    for(i=0; i<glines; i++){
        lines_inc = i < num_h_ceil ? h_trunc : h_trunc+1;
        
        tile_lines = lines_inc;
        reg_top = noborder_rt;
        if(noborder_rt > 0){ //check if there is a top border
            tile_lines++;
            reg_top--;
        }
        if(noborder_rt+lines_inc < h - 2){//check if there is a bottom border
            tile_lines++;
        }
        noborder_rl=0;
        for(j=0; j<gcolumns; j++){
            std::cout << "===============inner=======================\n";
            std::cout << x++ << "->" << i << "," << j <<"\n";
            std::cout << "======================================\n";
            columns_inc = j < num_w_ceil ? w_trunc : w_trunc+1;
            tile_columns = columns_inc;
            reg_left = noborder_rl;
            if(noborder_rl > 0){//check if there is a left border
                tile_columns++;
                reg_left--;
            }
            if(noborder_rl+columns_inc < w - 2){//check if there is a right border
                tile_columns++;
            }

            //std::cout << "filling: " << noborder_rt << "," << noborder_rl << "..." << noborder_rt + lines_inc << "," << noborder_rl + columns_inc <<"\n";

            
            std::cout << reg_left << "," << reg_top << "," << tile_columns << "," << tile_lines << "\n------------\n";
            std::cout << noborder_rl << "," << noborder_rt << "," << columns_inc << "," << tile_lines << "\n------------\n";

            maxtree *new_tree = new maxtree(tile_lines, tile_columns);
            vips::VRegion reg = in->region(reg_left, reg_top, tile_columns, tile_lines);

            reg.prepare(reg_left, reg_top, tile_columns, tile_lines);
            new_tree->fill_from_VRegion(reg, reg_top, reg_left, verbose);
            tiles.push_back(new_tree);
           //vips_region_invalidate(reg.get_region());

            std::cout << new_tree->to_string(GVAL,5);
            noborder_rl+=columns_inc;
        }
        noborder_rt+=lines_inc;
    }

    for(int i=0; i < tiles.size(); i++){
        t = tiles.at(i);
        std::cout << "tile:" << i << " " << tiles.at(i)->h << ", " <<  tiles.at(i)->w << "\n";
        t->compute_sequential_iterative();
        if(verbose){
            std::cout << "__________________GVAL________________\n";
            std::cout << t->to_string(GVAL,5);
            std::cout << "__________________PARENT________________\n";
            std::cout << t->to_string();
        }
    }

    //as boundary trees tem alturas e tamanhos distintos, logo é possível estimar o custo de um merge

    return 0;
}
