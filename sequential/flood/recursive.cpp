#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>
#include <deque>

#include "maxtree_node.hpp"
#include "utils.hpp"

#define INQUEUE -2

using namespace vips;
template<typename T, typename cmp>
class custom_priority_queue : public std::priority_queue<T, std::vector<T>, cmp>
{
  public:

        bool remove(const T& value) {
            //auto it= std::find(this->c.begin(), this->c.end(), value);
            auto it = this->c.begin(); 
            while(it != this->c.end()){
                if(*it == value){
                    break;
                }
                it++;
            }

            if (it == this->c.end()) {
                return false;
            }else if (it == this->c.begin()) {
                // deque the top element
                this->pop();
            }else{
                // remove element and re-heap
                this->c.erase(it);
                std::make_heap(this->c.begin(), this->c.end(), this->comp);
            }
            return true;
        }
};

void print_deque(std::deque<maxtree_node*> d){
    for(auto x: d){
        std::cout << *x;
    }
    std::cout << "\n";
}

void print_hq(std::vector<std::deque<maxtree_node*>> *hqueue){
    std::cout <<"===========QUEUE=============\n";
    for(auto d: *hqueue){
        if(!d.empty()){
            print_deque(d);
        }
    }
    std::cout<<"\n";
    std::cout <<"============================\n";
}

int flood(int lambda, maxtree_node *r, 
          std::vector<std::deque<maxtree_node*>> *hqueue,
          std::vector<maxtree_node*> *data,
          std::vector<maxtree_node*> *levroot,
          int h, int w){
    
    while(!hqueue->at(lambda).empty()){
        maxtree_node* p = hqueue->at(lambda).front();
        hqueue->at(lambda).pop_front();
        p->parent = r->idx;
        auto N = get_neighbours(p,data, h, w);
        for(auto n: N){
            if(n->parent == -1){
                int l = n->gval;
                if(levroot->at(l) == NULL){
                    levroot->at(l) = n;
                }
                hqueue->at(l).push_back(n);
                n->parent = INQUEUE;
                while(l>lambda){
                    l=flood(l,levroot->at(l),hqueue,data,levroot,h,w);
                }
            }
        }
        
    }
    levroot->at(lambda) = NULL;
    int lpar = lambda-1;
    while(lpar >= 0 && levroot->at(lpar) == NULL){
        lpar--;
    }
    if(lpar != -1){
        r->parent = levroot->at(lpar)->idx;
    }
    
    return lpar;
}

std::vector<maxtree_node*> *maxtree(VImage *in, int band = 0, int gl = 256){
    std::vector<maxtree_node*> *data;
    data = new std::vector<maxtree_node*>;
    int h=in->height();
    int w=in->width();
    int lmin;
    //std::vector<bool> *visited = new std::vector<bool>(h*w, false);
    //std::cout << visited->size() << " <--- visited size\n"; 
    std::vector<maxtree_node*> *levroot = new std::vector<maxtree_node*>(gl, NULL);
    std::vector<std::deque<maxtree_node*>> *hqueue = 
        new std::vector<std::deque<maxtree_node*>>(gl);
    maxtree_node *pmin;

    unsigned long long int idx=0;
    VImage img = in->copy_memory();
    VipsImage *pointer_image = img.get_image();
    VipsPel *vpel;
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            vpel = VIPS_IMAGE_ADDR(pointer_image,c,l);
            // std::cout << (int)*vpel << "\n";
            data->push_back(new maxtree_node((int) (*vpel), idx));
            idx++;
        }
    }
    std::cout << "fim leitura\n";
    //print_matrix(data,h,w);

    pmin = min_gval(data);
    lmin = pmin->gval;
    std::cout << lmin << "\n\n";
    hqueue->at(lmin).push_back(pmin);
    levroot->at(lmin) = pmin;
    flood(lmin, pmin, hqueue, data, levroot, h, w);

    //visited->at(nextpix->idx) = true;
    //std::cout << "-------> visiting: " << *nextpix << "\n";

  
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