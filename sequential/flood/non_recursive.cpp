#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <tuple>

using namespace vips;

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
        bool correct_filter;
        double out_value;
        double gval;

    maxtree_node(double g, double v = 0){
        this->parent = -1;
//        this->area = 1;
        this->correct_filter = false;
        this->out_value = v;
        this->gval = g;
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

void print_matrix(std::vector<maxtree_node> *m, int  h, int w){
    std::cout << h << ", " << w << "\n";
    std::cout << m->size()<<"\n";
    int l,c;
    for(l=0;l<h; l++){
        std::cout << l << ":";
        for(c=0;c<w; c++){
            std::cout << "("<< l << "," << c <<")";
            std::cout.width(4);
            std::cout << m->at(index_of(c,l,h,w)).parent;
        }
        std::cout << "\n";
    }
}

std::vector<unsigned int> get_neighbours(unsigned int pixel, int h, int w){
    std::vector<unsigned int> v;
    unsigned int pl, pc;
    std::tie(pl, pc) = lin_col(pixel, h, w);
    if(pl < (unsigned int)h - 1){
        v.push_back(index_of(pl+1, pc, h, w));
    }
    if(pl >= 1){
        v.push_back(index_of(pl-1, pc, h, w));
    }
    if(pc < (unsigned int)w - 1){
        v.push_back(index_of(pl, pc+1, h, w));
    }
    if(pc >= 1){
        v.push_back(index_of(pl, pc-1, h, w));
    }
    return v;
}


unsigned int min_gval(std::vector<maxtree_node> *t){
    unsigned int i, min = 0;
    for(i=1; i < t->size(); i++){
        if (t->at(i).gval < t->at(min).gval){
            min = i;
        }
    }
    return min;
}




std::vector<maxtree_node> *maxtree(VImage *in, int band = 0){
    std::vector<maxtree_node> *data;
    data = new std::vector<maxtree_node>;
    int h=in->height();
    int w=in->width();
    std::vector<bool> *visited = new std::vector<bool>;
    std::priority_queue<unsigned int> idx_pq;
    std::stack<unsigned int> idx_stack;
    unsigned int nextpix, smallest_idx, p,  t, stack_top;


    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            maxtree_node n(pval);
            data->push_back(n);
            visited->push_back(false);
        }
    }

    nextpix = smallest_idx = min_gval(data);
    idx_pq.push(smallest_idx);
    idx_stack.push(smallest_idx);
    std::cout << nextpix << " "<< smallest_idx;

    do{
        p=nextpix;
        std::vector<unsigned int> neighbours = get_neighbours(p,h,w);
        for(unsigned int q : neighbours){
            if(!visited->at(q)){
                idx_pq.push(q);
                visited->at(q) = true;
            }
            if(data->at(q).gval > data->at(p).gval){
                break;
            }
        }
        if(!idx_pq.empty()) nextpix = idx_pq.top();
        if(data->at(nextpix).gval > data->at(p).gval){
            idx_stack.push(nextpix);
        }else{
            idx_pq.pop();
            stack_top = idx_stack.top();
            if(p!=stack_top){
                data->at(p).parent=stack_top;
//                data->at(stack_top).area += data->at(p).area;
            }
            if(!idx_pq.empty()) nextpix = idx_pq.top();
            if(data->at(nextpix).gval < data->at(p).gval){
                do{
                    
                    if(!idx_stack.empty()) t = idx_stack.top();

                    idx_stack.pop();
                    if(p!=t){
                        data->at(p).parent=t;
//                        data->at(t).area += data->at(p).area;
                    }
                }while(data->at(t).gval > data->at(nextpix).gval);
            }
            if(!idx_stack.empty()) t = idx_stack.top();
        }

    }while(!idx_pq.empty());
    
    while (!idx_stack.empty()){
        t = idx_stack.top();
        idx_stack.pop();
        if(p!=t){
            data->at(p).parent=t;
//            data->at(t).area += data->at(p).area;
        }
    }

    return data;
}



int main(int argc, char **argv){
    VImage *in;
    std::vector<maxtree_node> *t;
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
    std::cout << "\n";
    return 0;
}