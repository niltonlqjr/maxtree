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

void process_stack(std::stack<maxtree_node*> &s, maxtree_node *r, maxtree_node *q){
    double l = q->gval;
    maxtree_node *t;
    s.pop();
    while(!s.empty() && l < s.top()->gval){
        //std::cout << l <<" "<< s.top()->gval << "\n";
        t = s.top();
        r->parent = t->idx;
        r = t;
    }
    if(s.empty() || s.top()->gval != l){
        s.push(q);
    }
    r->parent = s.top()->idx;

}

std::vector<maxtree_node*> *maxtree(VImage *in, int band = 0){
    std::vector<maxtree_node*> *data;
    data = new std::vector<maxtree_node*>;
    int h=in->height();
    int w=in->width();
    //std::vector<bool> *visited = new std::vector<bool>(h*w, false);
    //std::cout << visited->size() << " <--- visited size\n"; 
    custom_priority_queue<maxtree_node*, cmp_maxtree_nodes> pixel_pq;
    std::stack<maxtree_node*> pixel_stack;
    maxtree_node *q, *p, *r;

    unsigned int idx=0;
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            data->push_back(new maxtree_node(pval, idx));
            idx++;
        }
    }

    print_matrix(data,h,w);

    p = min_gval(data);
    p->parent = INQUEUE;
    pixel_pq.push(p);
    pixel_stack.push(p);

    //visited->at(nextpix->idx) = true;
    //std::cout << "-------> visiting: " << *nextpix << "\n";

    //int visited = 0;
    bool subir, descer;
    
    while(!pixel_pq.empty()){
        subir = false;
        descer = false;
        p = pixel_pq.top();
        r = pixel_stack.top();
        std::cout << "processando o pixel:" << *p << "\n";
        auto N=get_neighbours(p,data,h,w);
        for(maxtree_node *n: N){
            if(n->parent == -1){
                pixel_pq.push(n);
                n->parent = INQUEUE;
                if(p->gval < n->gval){
                    subir = true;
                    q=n;
                    break;
                }
            }
            
        }
        if(!subir){
            p->parent = r->idx;
            pixel_pq.remove(p);
            if(pixel_pq.size() <2 || 
                pixel_pq.top()->gval != pixel_pq.second()->gval){
                descer=true;
            }
            if(descer){
                if(!pixel_stack.empty()){
                    pixel_stack.pop();
                    if(!pixel_stack.empty()){
                        p->parent = pixel_stack.top()->idx;
                    }else{
                        p->parent = p->idx;
                    }
                }
            }
        }else{
            pixel_stack.push(q);
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