#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>
#include <deque>

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

class maxtree_node{
    public:
        int parent;
        int label;
        unsigned int idx;
        double gval;

    maxtree_node(double g, unsigned int i, double v = 0){
        this->parent = -1;
        this->idx = i;
//        this->area = 1;
        this->label = -1;
        this->gval = g;
    }

    std::string to_str(){
        std::string s;
        s = "(id:"+ std::to_string(this->idx) 
            +", parent:"+ std::to_string(this->parent) 
            +", gval:"+std::to_string(this->gval)+")";
        return s;
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


void label_components(std::vector<maxtree_node*> *mt){
    for(auto p: *mt){
        if(p->gval == mt->at(p->parent)->gval){
            p->label = p->parent;
        }else{
            p->label = p->idx;
        }
    }
}

struct cmp_maxtree_nodes{
    bool operator()(const maxtree_node* lhs, const maxtree_node* rhs) const
    {
        return lhs->gval < rhs->gval;
    }
};

std::ostream &operator<<(std::ostream &o, maxtree_node &n){
    o << "(idx:" << n.idx << " gval:"<< n.gval <<") ";
    return o;
 }


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

void print_labels(std::vector<maxtree_node*> *m, int  h, int w, bool metadata=false){
    std::cout << h << ", " << w << "\n";
    std::cout << m->size()<<"\n";
    int l,c;
    for(l=0;l<h; l++){
        if(metadata) std::cout << l << ":";
        for(c=0;c<w; c++){
            if(metadata) std::cout << "("<< l << "," << c <<")";
            std::cout.width(4);
            std::cout << m->at(index_of(l,c,h,w))->label << " ";
        }
        std::cout << "\n";
    }
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
            std::cout << m->at(index_of(l,c,h,w))->parent << " ";
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
    if(pl >= 1){
        idx = index_of(pl-1, pc, h, w);
        v.push_back(t->at(idx));
    }
    if(pl < (unsigned int)h - 1){
        idx = index_of(pl+1, pc, h, w);
        v.push_back(t->at(idx));
    }
    if(pc >= 1){
        idx = index_of(pl, pc-1, h, w);
        v.push_back(t->at(idx));
    }
    if(pc < (unsigned int)w - 1){
        idx = index_of(pl, pc+1, h, w);
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

    unsigned int idx=0;
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            data->push_back(new maxtree_node(pval, idx));
            idx++;
        }
    }

    print_matrix(data,h,w);

    pmin = min_gval(data);
    lmin = pmin->gval;
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