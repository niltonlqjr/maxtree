#include <vips/vips8>
#include <iostream>
using namespace vips;

VImage *maxtree(VImage *in){
    VImage *parents = NULL;
    return parents;
}

void print_VImage(VImage *in){
    double p;
    int h=in->height();
    int w=in->width();

    for(int l=0; l < h; l++){
        for(int c=0; c < w; c++){
            p = in->getpoint(c,l)[0];
            std::cout.width(4);
            std::cout << p;
        }
        std::cout << "\n";
    }

    return;
}


int main(int argc, char **argv){
    VImage *in;
    
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    print_VImage(in);
    
    

    vips_shutdown();
    std::cout << "\n";
    return 0;
}