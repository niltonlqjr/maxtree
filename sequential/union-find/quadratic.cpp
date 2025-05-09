#include <iostream>
#include <vips/vips8>

#include "utils.hpp"
#include "maxtree.hpp"
#include "maxtree_node.hpp"


int main(int argc, char *argv[]){
	vips::VImage *in;
    maxtree *t;
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new vips::VImage(vips::VImage::new_from_file(argv[1],NULL));
    int h,w;
    h=in->height();
    w=in->width();
    print_VImage_band(in);
	t = new maxtree(h,w);
	vips::VImage cp = in->copy_memory();
	t->fill_from_VImage(cp);
    
    std::cout << t->to_string(GVAL,5);
   
    
    
    return 0;
}
