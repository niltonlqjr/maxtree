#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vips/vips8>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include "maxtree_node.hpp"
#include "maxtree.hpp"
#include "utils.hpp"

using namespace vips;

class worker_status{
    public:
        double mem_size;
        double mem_freq;
        int num_cores;
        std::vector<double> cores_freq;

    worker_status(double mem_size, double mem_freq, int num_cores, std::vector<double> cores_freq){
        this->num_cores = num_cores;
        this->mem_size = mem_size;
        this->cores_freq = cores_freq;
        this->mem_freq = mem_freq;
    }

    double get_computation_power(){
        double freq_sum=0.0;
        double mem;
        for(auto f: this->cores_freq){
            freq_sum += f;
        }
        mem = this->mem_size * this->mem_freq;
        return abs(freq_sum + mem);
    }

    template<class Worker, class Container = std::vector<Worker>>
    double get_relative_power(Container workers){
        double max=-1;
        double wp;
        for(auto w: workers){
            wp = w.get_computation_power();
            if(wp>max){
                max = wp;
            }
        }
        return this->get_computation_power() / max;
    } 

};



class task{
    private:
        std::vector<int> *pixels_index;
        std::vector<bool> *visited;
        int parent_thread;
        int parent_pixel;
    public:
        int size;
        int threshold;
    task(int threshold, int parent_thread, int parent_pixel){
        this->parent_pixel = parent_pixel;
        this->parent_thread = parent_thread;
        this->size = 0;
        this->threshold = threshold;
        this->visited = new std::vector<bool>();
        this->pixels_index = new td::vector<int>()
    }
    void add_pixel(int p_idx){
        size += 1;
        pixels_index->push_back(p_idx);
        visited->push_back(false);
    }
    std::vector<bool> *get_visited(){
        return this->visited;
    }
    int get_task_pixel(int idx){
        // try{
        //     int ret = pixels_index->at(idx);
        // }catch (const std::out_of_range &e){
        //     std::cerr << "invalid task index " << idx << " \n" << e.what() << "\n";
        // }
        int ret = pixels_index->at(idx);
    }
};


template <class Task>
class bag_of_tasks{
    private:
        std::vector<Task> tasks;
    public:
        bag_of_tasks(){
            
        }
        ~bag_of_tasks(){
            
        }
        void insert_task(Task t, bool race_condition = false){
            tasks.push_back(t);
        }
        Task get_task(bool race_condition = false){
            Task r;
            std::swap(tasks->front(), tasks->back());
            r = tasks->back();
            tasks->pop_back();
            return r;
        }
        bool empty(){
            return this->tasks->size == 0;
        }
};


void print_map(std::map<std::string, std::string> *m){
    for(auto x: *m){
        std::cout << x.first << "=>" << x.second << "\n";
    }
}

bool is_blank(std::string s){
    std::vector<char> b = {' ', '\t', '\n', '\r'};
    for(int i=0; i<s.size(); i++){
        if(std::find(b.begin(), b.end(), s[i]) == b.end()){
            return false;
        }
    }
    return true;
}
std::map<std::string, std::string> *parse_config(char arg[]){
    std::ifstream f(arg);
    std::string l;
    std::map<std::string, std::string> *ret = new std::map<std::string, std::string>();
    while (!f.eof()){
        std::getline(f, l);
        bool blank = is_blank(l);
        if(!blank && l.front() != '#'){
            size_t spl_pos = l.find("=");
            std::string k = l.substr(0,spl_pos);
            std::string v = l.substr(spl_pos+1);
            (*ret)[k] = v;
        }
        
    }
    return ret;

}



std::vector<int> grow_region(maxtree *m, double threshold, int idx_ini, std::vector<bool> *visited){
    maxtree_node * f, *ini;
    std::vector<int> r;
    std::queue< maxtree_node*> q;

    ini = m->at_pos(idx_ini);

    if(!visited->at(idx_ini)){
        q.push(ini);
    }
    while(!q.empty()){
        f=q.front();
        q.pop();
        auto neighbours=m->get_neighbours(f->idx);
        for(auto n:neighbours){
            if(n->gval > threshold){
                if(!visited->at(n->idx)){
                    visited->at(n->idx) = true;
                    q.push(n);
                    r.push_back(n->idx);
                }
                
            }
        }
    }
    return r;

}

void maxtree_worker(bag_of_tasks<task> *bag, maxtree *m, bool *end){
    task *new_task;
    task *t;
    int idx_pixel, i;
    bool create_new_task;
    double next_threshold;
    while(!*end){
        t = &bag->get_task();
        next_threshold = t->threshold+1;
        create_new_task=false;
        for(i=0;i<t->size; i++)
            try{
                idx_pixel = t->get_task_pixel(i);
                if(m->at_pos(idx_pixel)->gval > next_threshold){
                    create_new_task=true;
                    break;
                }
            }catch(std::out_of_range &e){
                break;
            }
        }
        if(create_new_task){
            new_task = new task(next_threshold, 0, idx_pixel);
            auto p = m->at_pos(idx_pixel);
            auto new_task_pixels = grow_region(m,next_threshold,idx_pixel,t->get_visited());
        }
        
        
    }
}

maxtree *maxtree_main(VImage *in, int nth = 2){
    std::vector<std::thread*> threads;
    bag_of_tasks<task> *bag;
    std::map<int, maxtree_node*> *data = new std::map<int, maxtree_node*>();

    int x=0;
    // h=in->height();
    // w=in->width();

    for(int l=0;l<in->height();l++){
        for(int c=0;c<in->width();c++){
            double p = in->getpoint(c,l)[0];
            (*data)[x] = new maxtree_node(p,x);
            //std::cout << data->at(x)->idx << " ";
            x++;
        }
    }
    std::cout << "\n";
    maxtree *m = new maxtree(data, in->height(), in->width());
    auto visited = new std::vector<bool>(m->h * m->w,false);

    double th=0;
    std::vector<int> v;
    for(int i=0;i < m->h * m->w; i++){
        if(m->at_pos(i)->gval > th){
            v = grow_region(m,th,m->at_pos(i)->idx,visited);
            for(auto mtn: v){
                auto pos = m->lin_col(mtn);
                std::cout << "(" << std::get<0>(pos) << "," << std::get<1>(pos) << ") ";
            }
            if(v.size() > 0) std::cout << "\n";
        }
    }
    
    return NULL;
}


/*

Modelo generico:
Criar modelo de **Threads** que contemple:
    - informações do computador (processador, memória, etc) onde está sendo executada
    - Noção de seu poder de processamento no pool de Threads
    - A forma de obter essas informações é indideferente (dentro do fonte, arquivo de configuração, argumentos do programa)

Criar modelo de tarefas genérico que possui:
    - um método virtual para sua execução e trabalha junto ao modelo de Threads
    - Uma referência (heuristica ou exata) do custo da tarefa


Maxtree:
Ver a possibilidade das primeiras tarefas serem a criação dos componentes num threashold parametrizado. 
Jogar Sementes em pontos distribuidos pela imagem e crescer as regiões até encontrar todos componentes.
Buscar colocar um tamnho minimo de crescimento para cada tarefa.
*/

void test_workers(){
    std::vector<worker_status> workers;
    for(int i=0; i<10; i++){
        std::vector<double> freqs = std::vector<double>();
        int limite=(rand()%8+1)*2;
        for(int f=0;f<limite; f++){
            freqs.push_back(rand()%601+3000);
        }
        double mem_size = (rand()%8+1)*4;
        double mem_freq = (rand()%8+8)*333;
        worker_status w = worker_status(mem_size, mem_freq, freqs.size(), freqs);
        std::cout << mem_size << " " << mem_freq << " " 
                  << freqs.size() << "{";
        for(auto f:freqs){
            std::cout << " " << f;
        }
        std::cout << "}\n";
        workers.push_back(w);
    }
    std::cout<<"=========";
    for(auto w: workers){
        std::cout << "absolute:" << w.get_computation_power() << "\n";
        std::cout << "relative:" << w.get_relative_power<worker_status>(workers) << "\n";
    }
    std::cout << "\n=========\n";
}

int main(int argc, char **argv){
    VImage *in;
    maxtree *t;
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::map<std::string, std::string> *configs;


    
    configs = parse_config(argv[2]);
    test_workers();
    std::cout<<"+++++++++++++++++++++++++\n";
    print_map(configs);
    std::cout<<"+++++++++++++++++++++++++\n";
    int h,w,nth;
    nth = std::stoi(configs->at("threads"));
    std::cout << "nth:" << nth << "\n";
    h=in->height();
    w=in->width();
    print_VImage_band(in);


    t=maxtree_main(in);
    //print_matrix(t, h, w);
    //label_components(t);
    //print_labels(t, h, w);
    vips_shutdown();
    return 0;
}