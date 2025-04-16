#include <numeric>
#include <source_location>

/*======================== C++ includes ================================*/

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
#include <stdexcept>
#include <mutex>

#include <semaphore.h> // using semaphore.h for older versions of c++ compatibility

#include <cmath>
#include <cstdlib>

/*========================= Project includes ===============================*/

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
        std::map<int, bool> *visited;
    public:
        int parent_thread;
        int parent_pixel;
        int size;
        double threshold;

    task(int threshold, int parent_thread, int parent_pixel){

        this->parent_pixel = parent_pixel;
        this->parent_thread = parent_thread;
        this->size = 0;
        this->threshold = threshold;
        this->visited = new std::map<int, bool>();
        this->pixels_index = new std::vector<int>();
    }

    void operator delete(void *ptr){
        // std::cout << "delete task\n";
        task *self = (task*)ptr;
        delete self->pixels_index;
        delete self->visited;
    }
    
    void add_pixel(int p_idx){
        this->size += 1;
        this->pixels_index->push_back(p_idx);
        this->visited->emplace(p_idx,false);
    }

    std::vector<int> *get_all_pixels_ids(){
        return this->pixels_index;
    }

    std::map<int, bool> *get_visited(){
        return this->visited;
    }
    void print(){
        std::cout << "===============\n";

        std::cout << "parent pixel: " << this->parent_pixel << "\n";
        std::cout << "parent thread: " << this->parent_thread << "\n";
        std::cout << "size: " << this->size << "\n";
        std::cout << "threshold: " << this-> threshold << "\n";
        std::cout << "task pixels: ";
        for(auto p: *this->pixels_index){
            std::cout << p << " ";
        }

        std::cout << "\n===============\n";
    }
    int get_task_pixel(int idx){
        // try{
        //     int ret = pixels_index->at(idx);
        // }catch (const std::out_of_range &e){
        //     std::cerr << "invalid task index " << idx << " \n" << e.what() << "\n";
        // }
        int ret = pixels_index->at(idx);
        return ret;
    }
};

template <class Task>
class bag_of_tasks{
    private:
        std::vector<Task> *tasks;
        bool race_condition;
        std::mutex *lock;
        sem_t *semaphore;
        
    public:
        bag_of_tasks(bool race_condition = false){
            this->tasks = new std::vector<Task>();
            this->race_condition = race_condition;
            if(this->race_condition){
                this->lock = new std::mutex();

                this->semaphore = new sem_t();
                sem_init(this->semaphore, 0, 0);
            }

        }
        ~bag_of_tasks(){
            delete this->tasks;
            if(this->race_condition){
                delete this->lock;
                delete this->semaphore;
            }
        }
        void insert_task(Task t){
            if(!this->race_condition){
                tasks->push_back(t);
            }

        }
        Task get_task(){
            if(!this->race_condition){
                std::swap(tasks->front(), tasks->back());
                Task r = tasks->back();
                tasks->pop_back();
                return r;
            }
        }
        bool empty(){
            return this->tasks->size() == 0;
        }
};

bool is_running(std::vector<bool>*v){
    for(auto x: *v){
        if(x == true){
            return true;
        }
    }
    return false;
}

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

int grow_region(maxtree *m, int idx_ini, task *t, task *new_task){
    int cont=0;
    maxtree_node *f, *ini;
    std::queue< maxtree_node*> q;
    double threshold = new_task->threshold;
    std::map<int, bool> *visited = t->get_visited();
    std::vector<int> region;
    ini = m->at_pos(idx_ini);

    if(!visited->at(idx_ini)){
        visited->at(idx_ini) = true;
        q.push(ini);
        cont++;
    }
    while(!q.empty()){
        f=q.front();
        new_task->add_pixel(f->idx);
        q.pop();
        auto neighbours=m->get_neighbours(f->idx);
        for(auto n:neighbours){
            if(n->gval >= threshold){
                if(!visited->at(n->idx)){
                    visited->at(n->idx) = true;
                    q.push(n);
                    cont++;
                }
                
            }
        }
    }
    if(cont>1){
        std::cout << "region: ";
        for(auto idx: *(new_task->get_all_pixels_ids())){
            auto pos = m->lin_col(idx);
            std::cout << "(" <<std::get<0>(pos)<< "," << std::get<1>(pos) <<") ";
        }
        std::cout<<"\n";
    }
    return cont;
}

void maxtree_worker(unsigned int id, bag_of_tasks<task> *bag, maxtree *m, bool *end, std::vector<bool> *processing){
    task *new_task;
    task *t;
    int idx_pixel, i;
    bool create_new_task;
    double next_threshold;
    //while(!*end){
    bool repeat = true;
    int num_visited;

    while(is_running(processing)){
        
        if(!(bag->empty())){
            
            processing->at(id) = true;
            task aux_t = bag->get_task();
            t = new task(aux_t);
            i = 0;
            num_visited = 0;
            next_threshold = t->threshold+1;
            while(i < t->size){
                create_new_task=false;
                // std::cout<<" task size:" << t->size << "\n";
                try{
                    idx_pixel = t->get_task_pixel(i);
                    // std::cout << "pixel index" << idx_pixel << " \n";
                    if(m->at_pos(idx_pixel)->gval >= next_threshold){
                        create_new_task=true;
                    }
                }catch(std::out_of_range &e){
                    std::cout << e.what() << "\n";
                    break;
                }
                if(create_new_task){
                    // std::cout << "new task: next threshold: " << next_threshold << " idx: " << idx_pixel << "\n";
                    auto p = m->at_pos(idx_pixel);
                    new_task = new task(next_threshold, id, idx_pixel);
                    num_visited = grow_region(m,idx_pixel,t,new_task);
                    if(num_visited > 0){
                        new_task->print();
                        bag->insert_task(*new_task);
                    }else{
                        delete new_task;
                    }
                }
                i++;
            }
        }else{
            processing->at(id) = false;
            repeat=false;
        }     
    }
}



maxtree *maxtree_main(VImage *in, int nth = 1){
    std::vector<std::thread*> threads;
    bag_of_tasks<task> *bag = new bag_of_tasks<task>();
    std::map<int, maxtree_node*> *data = new std::map<int, maxtree_node*>();

    int x=0;
    // h=in->height();
    // w=in->width();
    task t1(0,-1,0);
    
    for(int l=0;l<in->height();l++){
        for(int c=0;c<in->width();c++){
            double p = in->getpoint(c,l)[0];
            (*data)[x] = new maxtree_node(p,x);
            //std::cout << data->at(x)->idx << " ";
            t1.add_pixel(x);
            x++;
        }
    }
    std::cout << "\n";
    maxtree *m = new maxtree(data, in->height(), in->width());
    std::map<int, bool> *visited = new std::map<int, bool>();
    std::vector<bool> *processing = new std::vector<bool>();
    threads = std::vector<std::thread*>();
    int tid;
    bool end=false;
    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    bag->insert_task(t1);
    for(tid = 0; tid<nth; tid++){
        processing->push_back(true);
        //maxtree_worker(tid, bag, m, &end, processing);
        threads.push_back(new std::thread(maxtree_worker,tid,bag,m,&end,processing));
    }
    for(auto th: threads){
        std::cout << th->get_id() << " join\n";
        th->join();
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

int main(int argc, char **argv){
    VImage *in;
    maxtree *t;
    if(argc < 3){
        std::cout << "usage:\n" << argv[0] << " <image file> <config file>\n";
        std::cout << "example:\n" << argv[0] << " input.png configs/config_test.txt\n";
        return 1;
    }
    if (VIPS_INIT(argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::map<std::string, std::string> *configs;


    
    configs = parse_config(argv[2]);
    test_workers();
    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    print_map(configs);
        std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    int h,w,nth;
    nth = std::stoi(configs->at("threads"));
    std::cout << "nth:" << nth << "\n";
    h=in->height();
    w=in->width();
    print_VImage_band(in);

    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    t=maxtree_main(in);
    //print_matrix(t, h, w);
    //label_components(t);
    //print_labels(t, h, w);
    vips_shutdown();
    return 0;
}