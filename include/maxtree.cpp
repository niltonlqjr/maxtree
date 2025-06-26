#include "maxtree.hpp"


component::component(uint64_t id, std::vector<int> p, int parent, int attr){
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

maxtree::maxtree(uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    // this->data = new std::unordered_map<int, maxtree_node*>();
    this->data = new std::vector<maxtree_node*>();
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(4,false);
}

// maxtree::maxtree(std::unordered_map<int, maxtree_node*> *data, int32_t h, int32_t w){
maxtree::maxtree(std::vector<maxtree_node*> *data, uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->data = data;
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(4,false);
}

maxtree::maxtree(std::vector <bool> borders, uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    // this->data = new std::unordered_map<int, maxtree_node*>();
    this->data = new std::vector<maxtree_node*>();
    this->levelroots = new std::vector<maxtree_node*>();
    this->tile_borders = new std::vector<bool>(borders);
}

maxtree::maxtree(std::vector<maxtree_node*> *data, std::vector<bool> borders, uint32_t h, uint32_t w, uint32_t grid_i, uint32_t grid_j){
    this->h = h;
    this->w = w;
    this->grid_i = grid_i;
    this->grid_j = grid_j;
    this->data = data;
    this->tile_borders = new std::vector<bool>(borders);
    this->levelroots = new std::vector<maxtree_node*>();
}

uint64_t maxtree::index_of(uint32_t l, uint32_t c){
    return l * this->w + c;
}

std::tuple<uint32_t,uint32_t> maxtree::lin_col(uint64_t index){
    return std::make_tuple(index / this->w, index % this->w);
}

maxtree_node *maxtree::at_pos(int64_t l, int64_t c){
    int32_t idx = this->index_of(l, c); 
    return this->data->at(idx);
}

maxtree_node *maxtree::at_pos(int64_t index){
    return this->data->at(index);
}

maxtree_node *maxtree::get_parent(uint64_t node_idx){
    if(node_idx < 0){
        return NULL;
    }
    maxtree_node *node  = this->at_pos(node_idx);
    if(node->parent < 0){
        return NULL;
    }
    return this->at_pos(node->parent);
}

// std::unordered_map<int, maxtree_node*> *maxtree::get_data(){
std::vector<maxtree_node*> *maxtree::get_data(){
    return this->data;
}

uint64_t maxtree::get_size(){
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
    uint64_t idx=0;
    

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

void maxtree::fill_from_VImage(vips::VImage &img_in, uint32_t global_nlines, uint32_t global_ncols, bool verbose){
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
    if(n->parent >= 0){
        n_parent = this->data->at(n->parent);
    }else{
        return n;
    }
    while(n->gval == n_parent->gval){
        n = this->data->at(n->parent);
        if(n->parent <= -1){
            break;
        }
        n_parent = this->data->at(n_parent->parent);
    }
    return n;
}


boundary_tree *maxtree::get_boundary_tree(uint8_t connectivity){
    boundary_tree *bound_tree;
    boundary_node *bound_parent, *current;
    maxtree_node *tn, *parent, *to_merge, *neighbour;
    uint32_t i, j, pidx; 

    if(connectivity != 4){
        std::cerr << "Connectivity different of 4-connect not implemented yet\n";
        exit(EX_SOFTWARE);
    }

    bound_tree = new boundary_tree(this->h, this->w, this->grid_i, this->grid_j);
    if(this->tile_borders->at(TOP_BORDER)){
        for(j=0; j<this->w; j++){
            to_merge = this->at_pos(1,j);    
            neighbour = this->at_pos(0,j);
            if(neighbour->gval < to_merge->gval){
                to_merge=neighbour;
            }
            tn = this->get_levelroot(to_merge);
            boundary_node n(to_merge,to_merge->idx,this->get_levelroot(to_merge)->idx);
            bound_tree->insert_element(n,TOP_BORDER);
            
            bound_tree->add_lroot_tree(tn,to_merge->idx,this->get_data());
            
        }
    }

    if(this->tile_borders->at(RIGHT_BORDER)){
        for(i=0; i<this->h; i++){
            to_merge = this->at_pos(i, this->w-2);
            neighbour = this->at_pos(i, this->w-1);
            if(neighbour->gval < to_merge->gval){
                to_merge=neighbour;
            }
            
            tn = this->get_levelroot(to_merge);
            boundary_node n(to_merge,to_merge->idx,this->get_levelroot(to_merge)->idx);
            bound_tree->insert_element(n,RIGHT_BORDER);
            bound_tree->add_lroot_tree(tn, to_merge->idx, this->get_data());
        }
    }

    if(this->tile_borders->at(BOTTOM_BORDER)){
        //for(j = this->w-1; (int32_t)j >= 0; j--){
        for(j=0; j < this->w; j++){
            to_merge = this->at_pos(this->h-2,j);
            neighbour = this->at_pos(this->h-1,j);
            if(neighbour->gval < to_merge->gval){
                to_merge = neighbour;
            }
            tn = this->get_levelroot(to_merge);
            boundary_node n(to_merge,to_merge->idx,this->get_levelroot(to_merge)->idx);
            bound_tree->insert_element(n, BOTTOM_BORDER);
            bound_tree->add_lroot_tree(tn, to_merge->idx, this->get_data());
            
        }
    }

    if(this->tile_borders->at(LEFT_BORDER)){
        //for(i=this->h-1; (int32_t)i >=0; i--){
        for(i=0; i < this->h; i++){
            to_merge = this->at_pos(i, 1);
            neighbour = this->at_pos(i, 0);
            if(neighbour->gval < to_merge->gval){
                to_merge=neighbour;
            }
            tn = this->get_levelroot(to_merge);
            boundary_node n(to_merge,to_merge->idx,this->get_levelroot(to_merge)->idx);
            bound_tree->insert_element(n,LEFT_BORDER);
            bound_tree->add_lroot_tree(tn, to_merge->idx, this->get_data());
        }
    }
    return bound_tree;
}


/*

get only bordernodes without consider the overlap

boundary_tree *maxtree::get_boundary_tree_no_overlap(uint8_t connectivity){
    /*
    border[0]       -------->    border[w-1]
    border[2h+2w-1] ^       |
                    |       |
                    |       |
                    |       |
    border[h+2w-1]  <------ v   border[h+w-1]
    */  
    /*
    if(connectivity != 4){
        std::cerr << "Connectivity different of 4-connect not implemented yet\n";
        exit(EX_SOFTWARE);
    }

    boundary_tree *bonud_tree = new boundary_tree();
    boundary_node *bound_parent, *current;
    maxtree_node *tn, *parent;
    uint32_t i, j, pidx; 
        
    for(j=0; j<this->w; j++){
        tn = this->at_pos(0,j); // get the maxtree node at position 0,j
        tn = this->get_levelroot(tn); // find for the levelroot of this node
        boundary_node n(tn, -1); // create boundary tree
        if(bonud_tree->insert_element(n)){
            //adding ancestors of tn
            current=bonud_tree->get_border_node(tn->idx);//get the added node
            while(current!=NULL){
                parent = this->get_parent(current->maxtree_idx); // get parent (stored in maxtree) of current boundary node
                if(parent != NULL){// if this node has a parent (not the tile root)
                    boundary_node bound_parent(parent,-1); // create the parent node to add on bondary tree
                    bonud_tree->insert_element(bound_parent);
                    pidx = parent->idx;
                }else{
                    pidx = -1;
                }
                current->boundary_parent = pidx;// update the parent of current node
                current=bonud_tree->get_border_node(pidx);// go to the parent and add its ancerstors
            }
        }
    }

    for(i=0; i<this->h; i++){
        tn = this->at_pos(i, this->w-1);
        tn = this->get_levelroot(tn);
        boundary_node n(tn,-1);
        if(bonud_tree->insert_element(n)){
            current=bonud_tree->get_border_node(tn->idx);
            while(current!=NULL){
                parent = this->get_parent(current->maxtree_idx);
                if(parent != NULL){
                    boundary_node bound_parent(parent,-1);
                    bonud_tree->insert_element(bound_parent);
                    pidx = parent->idx;
                }else{
                    pidx = -1;
                }
                current->boundary_parent = pidx;
                current=bonud_tree->get_border_node(pidx);
            }
        }
    }

    
    
    for( j=this->w-1; (int32_t) j>=0; j--){
        tn = this->at_pos(this->h-1,j);
        tn = this->get_levelroot(tn);
        boundary_node n(tn, -1);
        if(bonud_tree->insert_element(n)){
            current=bonud_tree->get_border_node(tn->idx);
            while(current!=NULL){
                parent = this->get_parent(current->maxtree_idx);
                if(parent != NULL){
                    boundary_node bound_parent(parent,-1);
                    bonud_tree->insert_element(bound_parent);
                    pidx = parent->idx;
                }else{
                    pidx = -1;
                }
                current->boundary_parent = pidx;
                current=bonud_tree->get_border_node(pidx);
            }
        }
        
    }

    for(i=this->h-1; (int32_t)i >= 0; i--){
        tn = this->at_pos(i,0);
        tn = this->get_levelroot(tn);
        boundary_node n(tn, -1);
        if(bonud_tree->insert_element(n)){
            //adding ancestors
            current=bonud_tree->get_border_node(tn->idx);
            while(current!=NULL){
                parent = this->get_parent(current->maxtree_idx);
                if(parent != NULL){
                    boundary_node bound_parent(parent,-1);
                    bonud_tree->insert_element(bound_parent);
                    pidx = parent->idx;
                }else{
                    pidx = -1;
                }
                current->boundary_parent = pidx;
                current=bonud_tree->get_border_node(pidx);
            }
        }
    }

    return bonud_tree;
}
/* */ 

 
void maxtree::fill_from_VRegion(vips::VRegion &reg_in, uint32_t base_h, uint32_t base_w, 
                                uint32_t l_tiles, uint32_t c_tiles, bool verbose){
    VipsRegion *c_region = reg_in.get_region();
    uint64_t global_idx;
    
    this->h = vips_region_height(c_region);
    this->w = vips_region_width(c_region);
    
    uint32_t noborder_h = this->h, noborder_w = this->w;

    int32_t ini_col=base_h, ini_line=base_h;

    if(this->tile_borders->at(LEFT_BORDER)){
        noborder_h--;
        ini_col++;
    }
    if(this->tile_borders->at(RIGHT_BORDER)){
        noborder_h--;
    }
    if(this->tile_borders->at(TOP_BORDER)){
        noborder_w--;
        ini_line++;
    }
    if(this->tile_borders->at(BOTTOM_BORDER)){
        noborder_w--;
    }
    auto tam_noborder_tile = noborder_h * noborder_w;


   /*  if(verbose){
        std::cout << "filling: " << base_h << "," << base_w << "..." << base_h+this->h << "," << base_w+this->w <<"\n";
    } */
    
    char aux_enum_c[][50] = {"VIPS_FORMAT_UCHAR", "VIPS_FORMAT_CHAR", "VIPS_FORMAT_USHORT","VIPS_FORMAT_SHORT",
         "VIPS_FORMAT_UINT"," VIPS_FORMAT_INT"," VIPS_FORMAT_FLOAT"," VIPS_FORMAT_COMPLEX"," VIPS_FORMAT_DOUBLE",
         "VIPS_FORMAT_DPCOMPLEX","VIPS_FORMAT_LAST"};
    
    for(int l = 0; l < this->h; l++){
        for(int c = 0; c < this->w; c++){
            if(verbose){
                std::cout << "accessing:" << "(" << l << "," << c << ")\n";
            }
            int x = this->index_of(l,c);
            //VipsPel *vpel__ = VIPS_IMAGE_ADDR(c_region, c, l);
            global_idx = ((base_h+l) * c_tiles) + (c+base_w);
            std::cout << "local:(" << l << "," << c << ") Global:(" << l+base_h << ","<< c+base_w << ")\n";
            VipsPel *vpel = VIPS_REGION_ADDR(c_region, c+base_w, l+base_h);
            this->data->push_back(new maxtree_node((*vpel), x, global_idx));
        }
    }
} 


void maxtree::insert_component(component c, double gval){
    auto comps = this->components.find(gval);
    if(comps == this->components.end()){
        this->components[gval] = std::vector<component>();
    }

    this->components[gval].push_back(c);

}

void maxtree::insert_component(std::vector<int> comp, int64_t parent, double threshold, uint64_t id){
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


std::string maxtree::to_string(enum maxtee_node_field field, bool colored, uint8_t spaces ){
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
    }else if(field == GLOBAL_IDX){
        for(int i=0; i < this->h; i++){
            for(int j=0; j < this->w; j++){
                auto point = this->data->at(this->index_of(i,j))->global_idx;
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

std::vector<maxtree_node*> maxtree::get_neighbours(uint64_t pixel, uint8_t con){
    std::vector<maxtree_node*> v;
    int64_t idx, pl, pc;
    std::tie(pl, pc) = this->lin_col(pixel);
    if(con == 4 || con == 8){
        if(pl >= 1){
            idx = index_of(pl-1, pc);
            v.push_back(this->data->at(idx));
        }
        if(pl < (uint64_t)h - 1){
            idx = index_of(pl+1, pc);
            v.push_back(this->data->at(idx));
        }
        if(pc >= 1){
            idx = index_of(pl, pc-1);
            v.push_back(this->data->at(idx));
        }
        if(pc < (uint64_t)w - 1){
            idx = index_of(pl, pc+1);
            v.push_back(this->data->at(idx));
        }
    }
    if(con == 8){
        if(pl >= 1 && pc >=1){
            idx = index_of(pl-1, pc-1);
            v.push_back(this->data->at(idx));
        }
        if(pl < (uint64_t)h - 1 && pc < (uint64_t)w - 1){
            idx = index_of(pl+1, pc+1);
            v.push_back(this->data->at(idx));
        }
        if(pc >= 1 && pl < (uint64_t)h - 1){
            idx = index_of(pl+1, pc-1);
            v.push_back(this->data->at(idx));
        }
        if(pc < (uint64_t)w - 1 && pl >= 1){
            idx = index_of(pl-1, pc+1);
            v.push_back(this->data->at(idx));
        }
    }
    return v;
}



void maxtree::filter(Tattribute a){
    //todo
}

