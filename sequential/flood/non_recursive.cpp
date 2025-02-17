#include <vips/vips8>
#include <iostream>
#include <vector>
#include <queue>
#include <stack>
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
        this->area = 1;
        this->correct_filter = false;
        this->out_value = v;
        this->gval = g;
    }

    bool operator<(const maxtree_node &n) const{
        return gval < n.gval;
    }
    bool operator<=(const maxtree_node &n) const{
        return gval <= n.gval;
    }

    bool operator>(const maxtree_node &n) const{
        return gval > n.gval;
    }
    bool operator>=(const maxtree_node &n) const{
        return gval >= n.gval;
    }

    
    bool operator==(const maxtree_node &n) const{
        return gval == n.gval;
    }
    bool operator!=(const maxtree_node &n) const{
        return gval != n.gval;
    }

};


int index_of(int c, int l, int h, int w){
    return l*w+c;    
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

void print_matrix(std::vector<maxtree_node> *m, int  w, int h){
    for(int l=0;l<h; l++){
        for(int c=0;c<w; c++){
            std::cout.width(4);
            std::cout << m->at(l*w+c).parent;
        }
        std::cout << "\n";
    }
}


unsigned int min_gval(std::vector<maxtree_node> *t){
    unsigned int i, min = 0;
    for(i=1; i < t->size(); i++){
        if (t->at(i) < t->at(min)){
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
    for(int l=0; l<h; l++){
        for(int c=0;c<w;c++){
            double pval = in->getpoint(c,l)[band];
            maxtree_node n(pval);
            data->push_back(n);
            visited->push_back(false);
        }
    }
    print_matrix(data, w, h);
    
    std::priority_queue<unsigned int> idx_pq;
    std::stack<unsigned int> idx_stack;
    
    unsigned int nextpix, smallest_idx, p;
    nextpix = smallest_idx = min_gval(data);

    idx_pq.push(smallest_idx);
    idx_stack.push(smallest_idx);
    std::cout << nextpix << " "<< p;
    /*do{
        p=nextpix;

    }while(!idx_pq.empty());
    */

    return data;
}



int main(int argc, char **argv){
    VImage *in;
    
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    print_VImage_band(in);
    maxtree(in);
    vips_shutdown();
    std::cout << "\n";
    return 0;
}