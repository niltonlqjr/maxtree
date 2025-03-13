#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>

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


void process_stack(maxtree_node *r, maxtree_node *q, 
                   std::vector<maxtree_node*> *data,
                   std::stack<maxtree_node*> &pixel_stack){
    auto lambda = q->gval;
    pixel_stack.pop();
    while(!pixel_stack.empty() && lambda < pixel_stack.top()->gval){
        auto stack_top = pixel_stack.top();
        pixel_stack.pop();
        r->parent = stack_top->idx;
        r = data->at(r->parent);
    }
    if(pixel_stack.empty() || pixel_stack.top()->gval != lambda){
        pixel_stack.push(q);
    }
    r->parent = pixel_stack.top()->idx;
}

std::vector<maxtree_node*> *maxtree(VImage *in, int band = 0){
    std::vector<maxtree_node*> *data;
    data = new std::vector<maxtree_node*>;
    int h=in->height();
    int w=in->width();
    std::vector<bool> *visited = new std::vector<bool>;
    //custom_priority_queue<maxtree_node*,cmp_maxtree_nodes> pixel_pq;
    std::priority_queue<maxtree_node*, std::vector<maxtree_node*> ,cmp_maxtree_nodes> pixel_pq;
    std::stack<maxtree_node*> pixel_stack;
    maxtree_node *xm, *nextpix, *p;

    unsigned int idx=0;
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            data->push_back(new maxtree_node(pval, idx));
            visited->push_back(false);
            idx++;
        }
    }

    xm= min_gval(data);
    pixel_pq.push(xm);
    pixel_stack.push(xm);
    nextpix = xm;
    int iter = 0;
    do{
        p = nextpix;
        /* std::cout << "----------------------" << ++iter << "----------------------\n";
        std::cout<< *p <<"\n";
        print_pq(pixel_pq);
        print_stack(pixel_stack);

        std::cout<<"_________________________________________________\n"; */
        auto N = get_neighbours(p, data, h, w);
        for(auto q: N){
            if(q->parent == -1){/* if q not visited */
                q->parent = INQUEUE;
                pixel_pq.push(q);
                if(q->gval > p->gval){
                    break;
                }
            }
        }
        nextpix = pixel_pq.top();

        if(nextpix->gval > p->gval){
            pixel_stack.push(nextpix);
        }else{
/*             if(p == pixel_pq.top()){
                std::cout << "removendo topo na iteracao:" << ++iter << " " << *p << "\n";
            } */
            //pixel_pq.remove(p);
            pixel_pq.pop();
            if(p!=pixel_stack.top()){
                p->parent = pixel_stack.top()->idx;
            }
            
            if(pixel_pq.empty()){
                nextpix=p;
            }else{
                nextpix = pixel_pq.top();
            }

            if(nextpix->gval < p->gval){
                
                while(!pixel_stack.empty() && nextpix->gval < pixel_stack.top()->gval){
                    auto st = pixel_stack.top();
                    pixel_stack.pop();
                    if(!pixel_stack.empty())
                        st->parent = pixel_stack.top()->idx;
                }
                if(pixel_stack.empty() || pixel_stack.top()->gval < nextpix->gval){
                    pixel_stack.push(nextpix);
                }
            }

        }

    }while(!pixel_pq.empty());
    maxtree_node *root = pixel_stack.top();
    root->parent = root->idx;
    std::cout << "-----------------END---------------\n";
    print_pq(pixel_pq);
    print_stack(pixel_stack);
    std::cout <<"____________________________________\n";
    return data;
}



int main(int argc, char **argv){
    VImage *in;
    std::vector<maxtree_node*> *t;
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));
    int h,w;
    h=in->height();
    w=in->width();
    print_VImage_band(in);
    t=maxtree(in);
    print_matrix(t, h, w);
    label_components(t);
    print_labels(t, h, w);
    vips_shutdown();
    return 0;
}