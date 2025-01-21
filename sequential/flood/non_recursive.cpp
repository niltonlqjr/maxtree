#include <vips/vips8>
#include <iostream>
using namespace vips;

VImage *maxtree(VImage *in){
    VImage *parents = NULL;
    return parents;
}


int main (int argc, char **argv){
    VImage *in, fin;
    int h, w;
    
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);
    
    
    fin = VImage::new_from_file(argv[1],NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));


    h=in->height();
    w=in->width();
    double p;

    for(int l=0; l < h; l++){
        for(int c=0; c < w; c++){
            p = in->getpoint(c,l)[0];
            std::cout.width(4);
            std::cout << p;
        }
        std::cout << "\n";
    }

    
    

    vips_shutdown ();
    std::cout << "\n";
    return 0;
}