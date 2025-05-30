#include "maxtree.hpp"


component::component(u_int64_t id, std::vector<int> p, int parent, int attr){
    this->pixels = p;
    this->parent = parent;
    this->attribute = attr;
    this->id = id;
}

std::vector<int> component::get_pixels_index(){
    return this->pixels;
}

std::string component::to_string(){
    std::string s;
    s+= "id: "+ std::to_string(this->id) +" parent: "+ std::to_string(this->parent) + " pixels: ";
    for(auto p: this->pixels){
        s += std::to_string(p) + " ";
    }
    return s;
}

void component::insert_pixel(int p){
    this->pixels.push_back(p);
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

void maxtree::fill_from_VImage(vips::VImage &img_in){
    this->h = img_in.height();
    this->w = img_in.width();
    vips::VImage img = img_in.copy_memory();
    auto img_pels = img.get_image();
    
    char aux_enum_c[][50] = {"VIPS_FORMAT_UCHAR", "VIPS_FORMAT_CHAR", "VIPS_FORMAT_USHORT","VIPS_FORMAT_SHORT",
         "VIPS_FORMAT_UINT"," VIPS_FORMAT_INT"," VIPS_FORMAT_FLOAT"," VIPS_FORMAT_COMPLEX"," VIPS_FORMAT_DOUBLE",
         "VIPS_FORMAT_DPCOMPLEX","VIPS_FORMAT_LAST"};
    if (img_pels->BandFmt > 0){
        std::cout << aux_enum_c[img_pels->BandFmt];
    }else {
        std::cout << "VIPS_FORMAT_NOTSET";
    }
    std::cout << "\n";
    
    for(int l = 0; l < this->h; l++){
        for(int c = 0; c < this->w; c++){
            int x = this->index_of(l,c);
            VipsPel *vpel = VIPS_IMAGE_ADDR(img_pels, c, l);
            this->data->push_back(new maxtree_node((*vpel),x));
        }
    }
}

maxtree_node *maxtree::at_pos(int index){
    return this->data->at(index);
}

void maxtree::insert_component(component c, double gval){
    auto comps = this->components.find(gval);
    if(comps == this->components.end()){
        this->components[gval] = std::vector<component>();
    }

    this->components[gval].push_back(c);

}

void maxtree::insert_component(std::vector<int> comp, int parent, double threshold, uint64_t id){
    auto comps = this->components.find(threshold);
    if(comps == this->components.end()){
        this->components[threshold] = std::vector<component>();
    }

    this->threshold_locks[threshold].lock();
    component new_comp = component(id, comp, parent);
    this->components[threshold].push_back(new_comp);
    this->threshold_locks[threshold].unlock();


    

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
        if(par_atu > 0 && this->data->at(par_atu)->gval < this->data->at(lower_idx)->gval && pidx != lower_idx){
            this->data->at(pidx)->parent = lower_idx;
        } 
    }
    
    this->data_lock.unlock();

}


std::string maxtree::to_string(enum maxtee_node_field field, bool colored, int spaces ){
    std::string r;
    //a lot of ifs with same for code inside to avoid branches inside for
    if(field == PARENT){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto point = this->data->at(this->index_of(i,j))->parent;
                if(colored)
                    r+=terminal_color_string(point % 8);
                r += fill(std::to_string(point), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if(field == LABEL){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto point = this->data->at(this->index_of(i,j))->parent;
                if(colored)
                    r+=terminal_color_string(point % 8);
                r += fill(std::to_string(point), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if(field == IDX){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto point = this->data->at(this->index_of(i,j))->idx;
                if(colored)
                    r+=terminal_color_string(point % 8);
                r += fill(std::to_string(point), spaces-1) + " " ;
            }
            r += "\n";
        }
    }else if (field == GVAL){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto dpoint = this->data->at(this->index_of(i,j))->gval;
                if(colored)
                    r+=terminal_color_string(dpoint / 31);
                r += fill(std::to_string(dpoint), spaces+5) + " " ;
            }
            r += "\n";
        }
    }
    r+=terminal_color_string(RESET);
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

void maxtree::filter(Tattribute a){
    //todo
}