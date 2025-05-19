#include "maxtree.hpp"


component::component(std::vector<int> p, int parent, int attr){
    this->pixels = p;
    this->parent = parent;
    this->attribute = attr;
}

std::string component::to_string(){
    std::string s;
    s+= "parent: "+ std::to_string(this->parent) + " pixels: ";
    for(auto p: this->pixels){
        s += std::to_string(p) + " ";
    }
    return s;
}



maxtree::maxtree(int h, int w){
    this->h = h;
    this->w = w;
    // this->data = new std::unordered_map<int, maxtree_node*>();
    this->data = new std::vector<maxtree_node*>();
}

// maxtree::maxtree(std::unordered_map<int, maxtree_node*> *data, int h, int w){
maxtree::maxtree(std::vector<maxtree_node*> *data, int h, int w){
    this->data = data;
    this->h=h;
    this->w=w;
}

maxtree_node *maxtree::at_pos(int l, int c){
    int idx = this->index_of(l, c);
    return this->data->at(idx);
}

// std::unordered_map<int, maxtree_node*> *maxtree::get_data(){
std::vector<maxtree_node*> *maxtree::get_data(){
    return this->data;
}

unsigned long long int maxtree::get_size(){
    return this->data->size();
}

void maxtree::fill_from_VImage(vips::VImage &img){
    this->h = img.height();
    this->w = img.width();
    auto img_pels = img.get_image();
    for(int l = 0; l < this->h; l++){
        for(int c = 0; c < this->w; c++){
            int x = this->index_of(l,c);
            VipsPel *vpel = VIPS_IMAGE_ADDR(img_pels, c, l);
            // (*this->data)[x] = new maxtree_node((int) *vpel, x);
            // this->data->emplace(std::make_pair(x,new maxtree_node((int)(*vpel),x)));
            this->data->push_back(new maxtree_node((int)(*vpel),x));
        }
    }
}

maxtree_node *maxtree::at_pos(int index){
    return this->data->at(index);
}

void maxtree::insert_component(std::vector<int> comp, int parent, double threshold){
    auto comps = this->components.find(threshold);
    if(comps == this->components.end()){
        this->components[threshold] = std::vector<component>();
    }
    this->threshold_locks[threshold].lock();
    component new_comp = component(comp, parent);
    this->components[threshold].push_back(new_comp);
    this->threshold_locks[threshold].unlock();


    //the logic for compoenet insertion on matrix is wrong.

    int lower_idx = comp[0];
    for(int pidx:comp){
        if( this->data->at(pidx)->gval < this->data->at(lower_idx)->gval){
            lower_idx = pidx;
        }
    }
    //try to speedup putting one lock per node, if necessary;
    this->data_lock.lock();
    for(int pidx: comp){
        
        int lbl_atu = this->data->at(pidx)->label;
        if(this->data->at(lbl_atu)->gval < this->data->at(lower_idx)->gval){
            this->data->at(pidx)->label = lower_idx;
        }
    }

    this->data->at(lower_idx)->parent = parent;
    for(int pidx: comp){
        int par_atu = this->data->at(pidx)->parent;
        if(this->data->at(par_atu)->gval < this->data->at(lower_idx)->gval && pidx != lower_idx){
            this->data->at(pidx)->parent = lower_idx;
        } 
    }

    
    
    this->data_lock.unlock();

}


std::string maxtree::to_string(enum maxtee_node_field field, int spaces){
    std::string r;
    //a lot of ifs with same for code inside to avoid branches inside for
    if(field == PARENT){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                r += fill(std::to_string(this->data->at(this->index_of(i,j))->parent), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if(field == LABEL){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                r += fill(std::to_string(this->data->at(this->index_of(i,j))->label), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if(field == IDX){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                r += fill(std::to_string(this->data->at(this->index_of(i,j))->idx), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if (field == GVAL){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                r += fill(std::to_string((int)this->data->at(this->index_of(i,j))->gval), spaces-1) + " " ;
            }
            r += "\n";
        }
    }
    return r;
}



std::vector<component> maxtree::components_at(double threshold){
    return this->components[threshold];
}
std::vector<double> maxtree::all_thresholds(){
    std::vector<double> ret;
    for(auto t: this->components){
        ret.push_back(t.first);
    }
    return ret;
}



std::vector<maxtree_node*> maxtree::get_neighbours(int pixel){
    std::vector<maxtree_node*> v;
    int idx, pl, pc;
    std::tie(pl, pc) = this->lin_col(pixel);
    if(pl >= 1){
        idx = index_of(pl-1, pc);
        v.push_back(this->data->at(idx));
    }
    if(pl < (unsigned int)h - 1){
        idx = index_of(pl+1, pc);
        v.push_back(this->data->at(idx));
    }
    if(pc >= 1){
        idx = index_of(pl, pc-1);
        v.push_back(this->data->at(idx));
    }
    if(pc < (unsigned int)w - 1){
        idx = index_of(pl, pc+1);
        v.push_back(this->data->at(idx));
    }
    return v;
}

std::vector<maxtree_node*> maxtree::get_neighbours(int pixel_line, int pixel_col){
    std::vector<maxtree_node*> v;
    int idx;
    if(pixel_line >= 1){
        idx = index_of(pixel_line-1, pixel_col);
        v.push_back(this->data->at(idx));
    }
    if(pixel_line < (unsigned int)h - 1){
        idx = index_of(pixel_line+1, pixel_col);
        v.push_back(this->data->at(idx));
    }
    if(pixel_col >= 1){
        idx = index_of(pixel_line, pixel_col-1);
        v.push_back(this->data->at(idx));
    }
    if(pixel_col < (unsigned int)w - 1){
        idx = index_of(pixel_line, pixel_col+1);
        v.push_back(this->data->at(idx));
    }
    return v;
}

int maxtree::index_of(int l, int c){
    return l * this->w + c;
}

std::tuple<int,int> maxtree::lin_col(int index){
    return std::make_tuple(index / this->w, index % this->w);
}