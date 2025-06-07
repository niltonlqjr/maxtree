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
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(4,false);
}

// maxtree::maxtree(std::unordered_map<int, maxtree_node*> *data, int h, int w){
maxtree::maxtree(std::vector<maxtree_node*> *data, int h, int w){
    this->h=h;
    this->w=w;
    this->data = data;
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(4,false);
}

maxtree::maxtree(std::vector <bool> borders, int h, int w){
    this->h = h;
    this->w = w;
    // this->data = new std::unordered_map<int, maxtree_node*>();
    this->data = new std::vector<maxtree_node*>();
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(borders);
}

maxtree::maxtree(std::vector<maxtree_node*> *data, std::vector<bool> borders, int h, int w){
    this->h=h;
    this->w=w;
    this->data = data;
    this->tile_borders = new std::vector<bool>(borders);
    this->levelroots = new std::vector<maxtree_node*>();
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


std::vector<maxtree_node *> *maxtree::get_levelroots(){
    return this->levelroots;
}

void maxtree::compute_sequential_iterative(){
    #define INQUEUE -2
    std::priority_queue<maxtree_node*, std::vector<maxtree_node*> ,cmp_maxtree_nodes> pixel_pq;
    std::stack<maxtree_node*> pixel_stack;
    maxtree_node *xm, *nextpix, *p;
    unsigned long long int idx=0;
    

    xm = min_gval(this->get_data());
    pixel_pq.push(xm);
    pixel_stack.push(xm);
    nextpix = xm;

    do{
        p = nextpix;
        auto N = this->get_neighbours(p->idx);
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
            pixel_pq.pop();
            if(p!=pixel_stack.top()){
                p->parent = pixel_stack.top()->idx;
                pixel_stack.top()->compute_attribute(p->attribute);
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
                    this->levelroots->push_back(st);
                    if(!pixel_stack.empty()){
                        st->parent = pixel_stack.top()->idx;
                        pixel_stack.top()->compute_attribute(st->attribute);
                    }
                }
                if(pixel_stack.empty() || pixel_stack.top()->gval < nextpix->gval){
                    pixel_stack.push(nextpix);
                }
            }

        }

    }while(!pixel_pq.empty());
    maxtree_node *root = pixel_stack.top();
    this->levelroots->push_back(root);
    root->parent = -1;
}

void maxtree::fill_from_VImage(vips::VImage &img_in, bool verbose){
    this->h = img_in.height();
    this->w = img_in.width();
    vips::VImage img = img_in.copy_memory();
    auto img_pels = img.get_image();
    
    char aux_enum_c[][50] = {"VIPS_FORMAT_UCHAR", "VIPS_FORMAT_CHAR", "VIPS_FORMAT_USHORT","VIPS_FORMAT_SHORT",
         "VIPS_FORMAT_UINT"," VIPS_FORMAT_INT"," VIPS_FORMAT_FLOAT"," VIPS_FORMAT_COMPLEX"," VIPS_FORMAT_DOUBLE",
         "VIPS_FORMAT_DPCOMPLEX","VIPS_FORMAT_LAST"};
    if(verbose){
        std::cout << "pixel format: ";
        if (img_pels->BandFmt > 0){
            std::cout << aux_enum_c[img_pels->BandFmt];
        }else {
            std::cout << "VIPS_FORMAT_NOTSET";
        }
        std::cout << "\n";
    }
    
    for(int l = 0; l < this->h; l++){
        for(int c = 0; c < this->w; c++){
            int x = this->index_of(l,c);
            VipsPel *vpel = VIPS_IMAGE_ADDR(img_pels, c, l);
            this->data->push_back(new maxtree_node((*vpel),x));
        }
    }
}

maxtree_node *maxtree::get_levelroot(maxtree_node *n){
    maxtree_node *n_parent;
    if(n->parent != -1){
        n_parent = this->data->at(n->parent);
    }else{
        return n;
    }
    while(n->gval == n_parent->gval){
        n = this->data->at(n->parent);
        if(n->parent == -1){
            break;
        }
        n_parent = this->data->at(n_parent->parent);
        
    }
    return n;
}

maxtree maxtree::get_boundary_tree(){

    
}

void maxtree::merge(maxtree to_merge){

}


 
void maxtree::fill_from_VRegion(vips::VRegion &reg_in, uint32_t base_h, uint32_t base_w, bool verbose){
    VipsRegion *c_region = reg_in.get_region();
    this->h = vips_region_height(c_region);
    this->w = vips_region_width(c_region);
    
   /*  if(verbose){
        std::cout << "filling: " << base_h << "," << base_w << "..." << base_h+this->h << "," << base_w+this->w <<"\n";
    } */
    
    char aux_enum_c[][50] = {"VIPS_FORMAT_UCHAR", "VIPS_FORMAT_CHAR", "VIPS_FORMAT_USHORT","VIPS_FORMAT_SHORT",
         "VIPS_FORMAT_UINT"," VIPS_FORMAT_INT"," VIPS_FORMAT_FLOAT"," VIPS_FORMAT_COMPLEX"," VIPS_FORMAT_DOUBLE",
         "VIPS_FORMAT_DPCOMPLEX","VIPS_FORMAT_LAST"};
    
    for(int l = 0; l < this->h; l++){
        for(int c = 0; c < this->w; c++){
/*             if(verbose){
                std::cout << "accessing:" << "(" << l << "," << c << ")\n";
            } */
            int x = this->index_of(l,c);
            //VipsPel *vpel__ = VIPS_IMAGE_ADDR(c_region, c, l);
            VipsPel *vpel = VIPS_REGION_ADDR(c_region, c+base_w, l+base_h);
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
        this->threshold_locks[threshold].lock();
        if(comps == this->components.end()){
            this->components[threshold] = std::vector<component>();
        }
        this->threshold_locks[threshold].unlock();
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
    /*this->data_lock.lock();
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
    
    this->data_lock.unlock();*/

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
                auto point = this->data->at(this->index_of(i,j))->label;
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
    }else if(field == LEVELROOT){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                maxtree_node *lroot = this->get_levelroot(this->data->at(this->index_of(i,j)));
                auto dpoint = lroot->idx;
                if(colored)
                    r+=terminal_color_string(dpoint % 8);
                r += fill(std::to_string(dpoint), spaces+5) + " " ;
            }
            r += "\n";
        }
    }else if(field == ATTRIBUTE){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto dpoint = this->data->at(this->index_of(i,j))->attribute;
                if(colored)
                    r+=terminal_color_string(dpoint % 8);
                r += fill(std::to_string(dpoint), spaces+5) + " " ;
            }
            r += "\n";
        }
    }

    r+=terminal_color_string(RESET);
    return r;
}

std::string maxtree::string_borders(){
    std::string ret="";
    if(this->tile_borders->at(LEFT_BORDER)) ret+="Left ";
    if(this->tile_borders->at(TOP_BORDER)) ret+="Top ";
    if(this->tile_borders->at(RIGHT_BORDER)) ret+="Rigth ";
    if(this->tile_borders->at(BOTTOM_BORDER)) ret+="Bottom ";
    if(ret == "") ret+="No Borders";
    return ret;
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



std::vector<maxtree_node*> maxtree::get_neighbours(int pixel, int con){
    std::vector<maxtree_node*> v;
    int idx, pl, pc;
    std::tie(pl, pc) = this->lin_col(pixel);
    if(con == 4 || con == 8){
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
    }
    if(con == 8){
        if(pl >= 1 && pc >=1){
            idx = index_of(pl-1, pc-1);
            v.push_back(this->data->at(idx));
        }
        if(pl < (unsigned int)h - 1 && pc < (unsigned int)w - 1){
            idx = index_of(pl+1, pc+1);
            v.push_back(this->data->at(idx));
        }
        if(pc >= 1 && pl < (unsigned int)h - 1){
            idx = index_of(pl+1, pc-1);
            v.push_back(this->data->at(idx));
        }
        if(pc < (unsigned int)w - 1 && pl >= 1){
            idx = index_of(pl-1, pc+1);
            v.push_back(this->data->at(idx));
        }
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