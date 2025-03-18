#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vips/vips8>
#include <vector>
#include <algorithm>
#include <threads>
#include <queue>
#include "maxtree_node.hpp"
#include "utils.hpp"

using namespace vips;

void print_map(std::map<std::string, std::string> *m){
    for(auto x: *m){
        std::cout << x.first << "=>" << x.second << "\n";
    }
}

bool is_blank(std::string s){
    std::vector<char> b = {' ', '\t', '\n', '\r'};
    for(int i=0; i<s.size(); i++){
        if(std::find(b.begin(), b.end(), s[i]) == b.end()){
            return false;
        }
    }
    return true;
}
std::map<std::string, std::string> *parse_config(char arg[]){
    std::ifstream f(arg);
    std::string l;
    std::map<std::string, std::string> *ret = new std::map<std::string, std::string>();
    while (!f.eof()){
        std::getline(f, l);
        bool blank = is_blank(l);
        if(!blank && l.front() != '#'){
            size_t spl_pos = l.find("=");
            std::string k = l.substr(0,spl_pos);
            std::string v = l.substr(spl_pos+1);
            (*ret)[k] = v;
        }
        
    }
    return ret;

}

void maxtree_worker(
    std::priority_queue<maxtree_node*, std::vector<maxtree_node*> ,cmp_maxtree_nodes> *pq,
    std::map<int, maxtree_node*> *data){
}

std::vector<maxtree_node*> *maxtree_main(VImage *in, int nth = 2){
    std::vector<std::thread*> threads;
    std::priority_queue<maxtree_node*, std::vector<maxtree_node*> ,cmp_maxtree_nodes> shared_pq;
    sstd::map<int, maxtree_node*> *data;
    return NULL;
}

int main(int argc, char **argv){
    VImage *in;
    std::map<int, maxtree_node*> *t;
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::map<std::string, std::string> *configs;

    configs = parse_config(argv[2]);
    std::cout<<"+++++++++++++++++++++++++\n";
    print_map(configs);
    std::cout<<"+++++++++++++++++++++++++\n";
    int h,w,nth;
    nth = std::stoi(configs->at("threads"));
    std::cout << "nth:" << nth << "\n";
    h=in->height();
    w=in->width();
    print_VImage_band(in);
    //t=maxtree(in);
    //print_matrix(t, h, w);
    //label_components(t);
    //print_labels(t, h, w);
    vips_shutdown();
    return 0;
}