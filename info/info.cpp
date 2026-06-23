#include <iostream>

#include "maxtree_node.hpp"
#include "boundary_tree.hpp"

using namespace vips;
bool verbose;
int main(){
    std::cout << "size of maxtree node:" << sizeof(maxtree_node) << "\n";
    std::cout << "size of boundary tree node:" << sizeof(boundary_node)<<"\n";
    std::cout << "size of attribute:" << sizeof(Tattribute) << "\n";
    std::cout << "size of pixel:" << sizeof(Tpixel_value) << "\n";
    std::cout << "size of processor power measure:" << sizeof(Tprocess_power) << "\n";
    std::cout << "size of worker attribute:" << sizeof(TWorkerAttr) << "\n";
    std::cout << "size of worker index: "<<  sizeof(TWorkerIdx) << "\n";

}