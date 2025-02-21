#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>

using namespace vips;

template<typename T>
class custom_priority_queue : public std::priority_queue<T, std::vector<T>>
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
          }
          if (it == this->c.begin()) {
              // deque the top element
              this->pop();
          }    
          else {
              // remove element and re-heap
              this->c.erase(it);
              std::make_heap(this->c.begin(), this->c.end(), this->comp);
         }
         return true;
     }
};


#define INQUEUE -2
/*
VImage *maxtree(VImage *in){
    VImage *parents = NULL;
    return parents;
}*/

class maxtree_node{
    public:
        int parent;
        int area;
        unsigned int idx;
        bool correct_filter;
        double out_value;
        double gval;

    maxtree_node(double g, unsigned int i, double v = 0){
        this->parent = -1;
        this->idx = i;
//        this->area = 1;
        this->correct_filter = false;
        this->out_value = v;
        this->gval = g;
    }

    bool operator>(const maxtree_node &r){
        return this->gval > r.gval;
    }
    bool operator>=(const maxtree_node &r){
        return this->gval >= r.gval;
    }
    bool operator<(const maxtree_node &r){
        return this->gval < r.gval;
    }
    bool operator<=(const maxtree_node &r){
        return this->gval <= r.gval;
    }
    bool operator==(const maxtree_node &r){
        return this->gval == r.gval;
    }
    bool operator!=(const maxtree_node &r){
        return this->gval != r.gval;
    }
};


unsigned int index_of(unsigned int l, unsigned int c, int h, int w){
    return l*w+c;    
}

std::tuple<unsigned int, unsigned int> lin_col(int index, int h, int w){
    return std::make_tuple(index/w, index%w);
}

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

void print_matrix(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false){
    std::cout << h << ", " << w << "\n";
    std::cout << m->size()<<"\n";
    int l,c;
    for(l=0;l<h; l++){
        if(metadata) std::cout << l << ":";
        for(c=0;c<w; c++){
            if(metadata) std::cout << "("<< l << "," << c <<")";
            std::cout.width(4);
            std::cout << m->at(index_of(l,c,h,w))->parent;
        }
        std::cout << "\n";
    }
}

std::vector<maxtree_node*> get_neighbours(maxtree_node *pixel, 
                                          std::vector<maxtree_node *> *t,
                                          unsigned int h, unsigned int w){
    std::vector<maxtree_node*> v;
    unsigned int idx, pl, pc;
    std::tie(pl, pc) = lin_col(pixel->idx, h, w);
    if(pl < (unsigned int)h - 1){
        idx = index_of(pl+1, pc, h, w);
        v.push_back(t->at(idx));
    }
    if(pl >= 1){
        idx = index_of(pl-1, pc, h, w);
        v.push_back(t->at(idx));
    }
    if(pc < (unsigned int)w - 1){
        idx = index_of(pl, pc+1, h, w);
        v.push_back(t->at(idx));
    }
    if(pc >= 1){
        idx = index_of(pl, pc-1, h, w);
        v.push_back(t->at(idx));
    }
    return v;
}


maxtree_node *min_gval(std::vector<maxtree_node*> *t){
    unsigned int i; 
    maxtree_node *min = t->at(0);
    
    for(i=1; i < t->size(); i++){
        if (t->at(i)->gval < min->gval){
            min = t->at(i);
        }
    }
    return min;
}




std::vector<maxtree_node*> *maxtree(VImage *in, int band = 0){
    std::vector<maxtree_node*> *data;
    data = new std::vector<maxtree_node*>;
    int h=in->height();
    int w=in->width();
    std::vector<bool> *visited = new std::vector<bool>;
    custom_priority_queue<maxtree_node*> pixel_pq;
    std::stack<maxtree_node*> pixel_stack;
    maxtree_node *nextpix, *p, *t, *stack_top;

    unsigned int idx=0;
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            data->push_back(new maxtree_node(pval, idx));
            visited->push_back(false);
            idx++;
        }
    }

    nextpix = min_gval(data);
    pixel_pq.push(nextpix);
    pixel_stack.push(nextpix);
    int __iter=0;
    do{
        
        p=nextpix;
        std::vector<maxtree_node *> neighbours = get_neighbours(p,data,h,w);
        for(maxtree_node *q : neighbours){
            if(!visited->at(q->idx)){
                pixel_pq.push(q);
                visited->at(q->idx) = true;
            }
            if(q->gval > p->gval){
                break;
            }
        }
        if(!pixel_pq.empty()) nextpix = pixel_pq.top();
        if(nextpix->gval > p->gval){
            pixel_stack.push(nextpix);
        }else{
            pixel_pq.remove(p);
            if (!pixel_stack.empty()) stack_top = pixel_stack.top();
            if(p!=stack_top){
                p->parent=stack_top->idx;
//                data->at(stack_top).area += data->at(p).area;
            }
            if(!pixel_pq.empty()){
                nextpix = pixel_pq.top();
            } else{
                nextpix = p;
            }
            if(nextpix->gval < p->gval){
                do{
                    
                    //if(!pixel_stack.empty()) t = pixel_stack.top();

                    if(!pixel_stack.empty()) pixel_stack.pop();
                    if(p!=t){
                        p->parent=t->idx;
//                        data->at(t).area += data->at(p).area;
                    }
                }while(t->gval > nextpix->gval && !pixel_stack.empty());
            }
            if(!pixel_stack.empty()) t = pixel_stack.top();
            if(stack_top->gval < nextpix->gval){
                pixel_stack.push(nextpix);
            }
        }

    }while(!pixel_pq.empty());
    
    while (!pixel_stack.empty()){
        t = pixel_stack.top();
        pixel_stack.pop();
        if(p!=t){
            p->parent=t->idx;
//            data->at(t).area += data->at(p).area;
        }
    }

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
    vips_shutdown();
    return 0;
}