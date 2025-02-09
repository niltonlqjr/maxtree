#include <vips/vips8>
#include <iostream>
#include <vector>

using namespace vips;

#define INQUEUE -2
/*
VImage *maxtree(VImage *in){
    VImage *parents = NULL;
    return parents;
}*/

void print_VImage_band(VImage *in, int band = 0){
    double p;
    int h=in->height();
    int w=in->width();

    for(int l=0; l < h; l++){
        for(int c=0; c < w; c++){
            p = in->getpoint(c,l)[band];
            std::cout.width(4);
            std::cout << p;
        }
        std::cout << "\n";
    }

    return;
}

void print_matrix(std::vector<std::vector<int>*> *m){
    
    for(int l=0;l<m->size(); l++){
        for(int c=0;c<m->at(l)->size(); c++){
            std::cout.width(4);
            std::cout << m->at(l)->at(c);
        }
        std::cout << "\n";
    }
}


std::vector<std::vector<int>*> *maxtree(VImage *in){
    std::vector<std::vector<int>*> *parent;
    parent = new std::vector<std::vector<int>*>;
    int h=in->height();
    int w=in->width();
    for(int l=0; l<h; l++){
        std::vector<int> *line;
        line = new std::vector<int>;
        for(int c=0;c<w;c++){
            line->push_back(-1);
        }
        parent->push_back(line);
    }
    print_matrix(parent);
    return parent;
}



int main(int argc, char **argv){
    VImage *in;
    
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    print_VImage_band(in);
    maxtree(in);
    vips_shutdown();
    std::cout << "\n";
    return 0;
}