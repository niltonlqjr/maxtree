#include <numeric>
#include <source_location>

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vips/vips8>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include <limits>
#include <mutex>
#include <condition_variable>

#include "maxtree_node.hpp"
#include "maxtree.hpp"
#include "heap.hpp"
#include "utils.hpp"
#include "bag_of_task.hpp"

bool verbose;

using namespace vips;

maxtree *maxtree_main(VImage *in, int nth = 1){
    maxtree *m = new maxtree(in->height(), in->width());
    return m;
}

int main(int argc, char **argv){
    VImage *in;
    maxtree *t;
    
    if(argc < 3){
        std::cout << "usage:\n" << argv[0] << " <image file> <config file>\n";
        std::cout << "example:\n" << argv[0] << " input.png configs/config_test.txt\n";
        return 1;
    }
    if (VIPS_INIT(argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::unordered_map<std::string, std::string> *configs;

    configs = parse_config(argv[2]);

    int h,w,nth;
    verbose = false;

    nth = std::stoi(configs->at("threads"));
    auto conf_verbose = configs->find("verbose");
    if(conf_verbose != configs->end()){
        if(conf_verbose->second == "true"){
            verbose = true;
        }
    }

    if(argc == 4) nth = std::stoi(argv[3]);
    
    print_unordered_map(configs);
    h=in->height();
    w=in->width();
    if(verbose){
        print_VImage_band(in);
    }
    
    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    
    t=maxtree_main(in,nth);

    if(verbose){    
        std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";

        for(auto thold: t->all_thresholds()){
            std::vector<component> comps = t->components_at(thold);
            std::cout << "threshold: " << thold << "\n";
            for(int i=0;i<comps.size(); i++){
                std::cout << comps[i].to_string() << "\n===================\n";
            }
        }
    }
    if(verbose){ 
        print_VImage_band(in);
        std::cout << "=====================labels=====================\n";
        std::cout << t->to_string(LABEL) << "\n================================\n";
        std::cout << "=====================parents=====================\n";
        std::cout << t->to_string();
    }
    vips_shutdown();
    return 0;
}


