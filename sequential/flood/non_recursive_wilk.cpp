#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>
#include <string>
#include <ostream>

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

        T second(){
            return this->c.at(1);
        }

        void print(){
            for(T n: this->c){
                std::cout << *n;
            }
            std::cout << "\n";
        }

};


/*
VImage *maxtree(VImage *in){
    VImage *parents = NULL;
    return parents;
}*/
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



void print_pq(custom_priority_queue<maxtree_node*, cmp_maxtree_nodes> pq){
    std::cout <<"===========QUEUE=============\n";
    while(!pq.empty()){
        auto e=pq.top();
        std::cout<<*e<<" ";
        pq.remove(e);
    }
    std::cout<<"\n";
    std::cout <<"============================\n";
}

void print_stack(std::stack<maxtree_node*> s){
    std::cout <<"=========STACK===========\n";
    while(!s.empty()){
        std::cout << *(s.top()) << " ";
        s.pop();
    }
    std::cout<<"\n";
    std::cout <<"============================\n";
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
            std::cout << m->at(index_of(l,c,h,w))->parent;
        }
        std::cout << "\n";
    }
}

void label_components(std::vector<maxtree_node*> *mt){
    for(auto p: *mt){
        if(p->gval == mt->at(p->parent)->gval){
            p->label = p->parent;
        }else{
            p->label = p->idx;
        }
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
    custom_priority_queue<maxtree_node*,cmp_maxtree_nodes> pixel_pq;
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
            pixel_pq.remove(p);
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