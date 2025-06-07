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

    std::cout << "h trunc:" << h_trunc << "\n";
    std::cout << "w trunc:" << w_trunc << "\n";
    std::cout << "num_h_ceil:" << num_h_ceil << "\n";
    std::cout << "num_w_ceil:" << num_w_ceil << "\n";
    
    std::vector<std::vector<maxtree *>> tiles;
    bool left, top, right, bottom;
    uint32_t i,j,x = 0;
    uint32_t noborder_rt=0, noborder_rl, lines_inc, columns_inc; // original tiles (without borders) size variables
    uint32_t reg_top, reg_left, tile_lines, tile_columns; // tiles used by algorithm (with border) size variables
    for(i=0; i<glines; i++){
        std::vector<bool> borders(4,false);
        lines_inc = i < num_h_ceil ? h_trunc +1 : h_trunc;
        
        tile_lines = lines_inc;
        reg_top = noborder_rt;
        if(noborder_rt > 0){ //check if there is a top border
            tile_lines++;
            reg_top--;
            borders.at(TOP_BORDER) = true;
        }
        if(noborder_rt+lines_inc < h - 1){//check if there is a bottom border
            tile_lines++;
            borders.at(BOTTOM_BORDER) = true;
        }
        noborder_rl=0;
        tiles.push_back(std::vector<maxtree *>());
        for(j=0; j<gcolumns; j++){
            borders.at(LEFT_BORDER) = false;
            borders.at(RIGHT_BORDER) = false;
            std::cout << "===============inner loop=======================\n";
            std::cout << x++ << "->" << i << "," << j <<"\n";
            columns_inc = j < num_w_ceil ? w_trunc+1 : w_trunc;
            tile_columns = columns_inc;
            reg_left = noborder_rl;
            if(noborder_rl > 0){//check if there is a left border
                tile_columns++;
                reg_left--;
                borders.at(LEFT_BORDER) = true;
            }
            if(noborder_rl+columns_inc < w - 1){//check if there is a right border
                tile_columns++;
                borders.at(RIGHT_BORDER) = true;
            }
            
            //std::cout << "filling: " << noborder_rt << "," << noborder_rl << "..." << noborder_rt + lines_inc << "," << noborder_rl + columns_inc <<"\n";
            if(verbose){
                std::cout << "with borders:\n";
                std::cout << reg_left << "," << reg_top << "," << tile_columns << "," << tile_lines << "\nno borders: \n";
                std::cout << noborder_rl << "," << noborder_rt << "," << columns_inc << "," << lines_inc << "\n------------\n";
            }
            
            maxtree *new_tree = new maxtree(borders,tile_lines, tile_columns);
            vips::VRegion reg = in->region(reg_left, reg_top, tile_columns, tile_lines);
            
            reg.prepare(reg_left, reg_top, tile_columns, tile_lines);
            new_tree->fill_from_VRegion(reg, reg_top, reg_left, verbose);
            tiles.at(i).push_back(new_tree);
            vips_region_invalidate(reg.get_region());
            
            //std::cout << new_tree->to_string(GVAL,5);
            noborder_rl+=columns_inc;
            std::cout << "======================================\n";
        }
        noborder_rt+=lines_inc;
    }


    for(int i=0; i < glines; i++){
        for(int j=0;j<gcolumns; j++){
            t = tiles.at(i).at(j);
            if (verbose) std::cout << "tile:" << i << ", " << j << " size:" << t->h << ", " <<  t->w << "\n";
            t->compute_sequential_iterative();
            if(verbose){
                std::cout << "__________________GVAL________________\n";
                std::cout << t->to_string(GVAL,true,5);
                std::cout << "_________________PARENT________________\n";
                std::cout << t->to_string();
                std::cout << "________________LEVELROOT________________\n";
                std::cout << t->to_string(LEVELROOT,true,5);
                std::cout << "________________ATTRIBUTE________________\n";
                std::cout << t->to_string(ATTRIBUTE,false,5);

                std::cout << "_____________LEVELROOT vector___________\n";
                for(auto r: *(t->get_levelroots())){
                    std::cout << r->idx << " ";
                }

                std::cout << t->string_borders() << "\n";
                std::cout << "\n";

                
                

            }
        }
    }

    //as boundary trees tem alturas e tamanhos distintos, logo é possível estimar o custo de um merge

    return 0;
}
