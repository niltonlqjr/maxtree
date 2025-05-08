#include <numeric>
#include <source_location>

/*======================== C++ includes ================================*/

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vips/vips8>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include <limits>
#include <stdexcept>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

#include <semaphore.h>


#include <cmath>
#include <cstdlib>

/*========================= Project includes ===============================*/

#include "maxtree_node.hpp"
#include "maxtree.hpp"
#include "heap.hpp"
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
        std::vector<int> pixels_index;
        std::unordered_map<int, bool> visited;
    public:
        int parent_thread;
        int parent_pixel;
        int size;
        double threshold;

    void visit(int idx){
        this->visited.at(idx) = true;
    }

    bool visited_at(int idx){
        return this->visited.at(idx);
    }

    bool operator<(const task &r){
        return this->size < r.size;
    }

    bool operator>(const task &r){
        return this->size > r.size;
    }

    bool operator==(const task &r){
        return this->size == r.size;
    }

    task(int threshold = 0, int parent_thread = 0, int parent_pixel = 0){

        this->parent_pixel = parent_pixel;
        this->parent_thread = parent_thread;
        this->size = 0;
        this->threshold = threshold;
        //this->visited = new std::unordered_map<int, bool>();
        //this->pixels_index = new std::vector<int>();
    }

    friend std::ostream& operator<<(std::ostream& os, const task& t){
        os  << "parent pixel: " << t.parent_pixel << " ";
        os  << "parent thread: " << t.parent_thread << " ";
        os << "size: " << t.size << " ";
        os << "threshold: " << t.threshold << " ";
        os << "task pixels: ";
        for(auto p: t.pixels_index){
            os << p << " ";
        }
        return os;
    }

   /*  void operator delete(void *ptr){
        
        task *self = (task*)ptr;
        delete self->pixels_index;
        delete self->visited;
    } */
    
    int weight(){
        return this->size;
    }

    void add_pixel(int p_idx){
        this->size += 1;
        this->pixels_index.push_back(p_idx);
        this->visited.emplace(p_idx,false);
    }

    std::vector<int> get_all_pixels_ids(){
        return this->pixels_index;
    }

/*     std::unordered_map<int, bool> get_visited(){
        return this->visited;
    } */
    void print(){
        std::cout << "===============\n";

        std::cout << "parent pixel: " << this->parent_pixel << " ";
        std::cout << "parent thread: " << this->parent_thread << " ";
        std::cout << "size: " << this->size << " ";
        std::cout << "threshold: " << this-> threshold << " ";
        std::cout << "task pixels: ";
        for(auto p: this->pixels_index){
            std::cout << p << " ";
        }

        std::cout << "\n===============\n";
    }
    int get_task_pixel(int idx){
        int ret = this->pixels_index.at(idx);
        return ret;
    }
};

template <class Task>
class bag_of_tasks{
    private:
        max_heap<Task> *tasks;
        std::mutex lock;
        std::condition_variable has_task, no_task;
        int num_task;
        bool running;
        int waiting;

    public:
        
        bag_of_tasks(){
            this->tasks = new max_heap<Task>();
            this->num_task = 0;
            this->running = true;
            this->waiting = 0;
        }

        ~bag_of_tasks(){
            delete this->tasks;
        }

        void insert_task(Task t){
            std::unique_lock<std::mutex> l(this->lock);
            this->tasks->insert(t);
            this->num_task++;
            this->wakeup_workers(false);
        }
        
        int position_of(int priority){
            /*elaborar uma formula para obter a posição dada uma prioridade
            pensar numa forma de limitar as prioridades;*/
            return priority;
        }

        bool is_running(){
            return this->running;
        }

        bool get_task(Task &ret, int priority = -1){
            std::unique_lock<std::mutex> l(this->lock);
            int pos;
            if(priority == -1){
                pos = 0;
            }else{
                pos = this->position_of(priority);
            }
            while(this->num_task == 0 && this->running){
                this->waiting++;
                this->has_task.wait(l);
            }
            if(!this->running){
                return false;
            }else{
                ret = this->tasks->at(pos);
                this->tasks->remove_at(pos);
                this->num_task--;
                this->no_task.notify_all();
                return true;
            }
            
        }

        void wait_empty(){
            std::unique_lock<std::mutex> l(this->lock);
            while(this->num_task > 0){
                this->no_task.wait(l);
            }
        }

        void wakeup_workers(bool lock = true){
            if(lock){
                std::unique_lock<std::mutex> l(this->lock);
            }
            this->waiting=0;
            this->has_task.notify_all();
        }

        void notify_end(){
            this->num_task = 0;
            this->running = false;
            this->wakeup_workers();
        }
        
        int num_waiting(){
            return this->waiting;
        }

        void print(){
            this->tasks->print();
        }

        bool empty(){
            std::unique_lock<std::mutex> l(this->lock);
            return this->tasks->size() == 0;
        }
};



int grow_region(maxtree *m, int idx_ini, task *t, task *new_task, double threshold){
    int cont=0;
    maxtree_node *f, *ini;
    std::queue< maxtree_node*> q;
    //std::unordered_map<int, bool> *visited = t->get_visited();
    std::vector<int> region;
    ini = m->at_pos(idx_ini);

    if(!t->visited_at(idx_ini)){
        t->visit(idx_ini);
        q.push(ini);
        cont++;
    }
    while(!q.empty()){
        f=q.front();
        new_task->add_pixel(f->idx);
        q.pop();
        auto neighbours=m->get_neighbours(f->idx);
        for(auto n:neighbours){
            if(n->gval >= threshold){// the neighbor is in the region
                if(!t->visited_at(n->idx)){
                    t->visit(n->idx);
                    q.push(n);
                    cont++;
                }  
            }else{// the neighbor isn't in the region, so it can be the region parent
                if(new_task->parent_pixel == -1){//no parent for the region was assigned yet
                    new_task->parent_pixel = n->idx;
                }
                if(n->gval > m->at_pos(new_task->parent_pixel)->gval){
                    //we have a father, its component is in a higher level than this neighbour
                    new_task->parent_pixel = n->idx;
                }
            }
        }
    }
    return cont;
}

void maxtree_worker(unsigned int id, bag_of_tasks<task> *bag, maxtree *m) {
    task *new_task;
    task *t = new task();
    std::unordered_map<int, bool> *visited;
    int idx_pixel, i, num_visited;
    bool create_new_task, has_task;
    double next_threshold;
    
    while(bag->is_running()){
        has_task = bag->get_task(*t);
        if(!has_task){
            continue;
        }
        i = 0;
        num_visited = 0;
        next_threshold = t->threshold+1;
        //t->print();
        while(i < t->size){
            create_new_task=false;
            idx_pixel = t->get_task_pixel(i);
            try{
                if(m->at_pos(idx_pixel)->gval >= next_threshold){
                    create_new_task=true;
                }
            }catch(...){
                std::cout << " idx_pixel:" << idx_pixel << " i:" << i << "LINE: "<< __LINE__<<"\n";
                //t->print();
                break;
            }
            if(create_new_task){
                new_task = new task(next_threshold, id, -1);
                num_visited = grow_region(m,idx_pixel,t,new_task,next_threshold);
                if(num_visited > 0){ 
                    if(new_task->size != t->size){
                        //new_task->print();
                        m->insert_component(new_task->get_all_pixels_ids(),new_task->parent_pixel,next_threshold);
                        //std::cout << "task inserted\n";
                    }
                    bag->insert_task(*new_task);
                }else{
                    delete new_task;
                }
            }
            i++;
        }
    }
}




maxtree *maxtree_main(VImage *in, int nth = 1){
    std::vector<std::thread*> threads;
    bag_of_tasks<task> *bag = new bag_of_tasks<task>();
    std::unordered_map<int, maxtree_node*> *data = new std::unordered_map<int, maxtree_node*>();
    int x=0;
    double p;
    VImage *del = in;
    task t0 = task(0,-1,0);


    VipsPel *vpel;
    VipsImage *v = in->get_image();
    VImage img = in->copy_memory();//vips_image_copy_memory(v);
    // look at a faster way to read image with vips
    std::cout << "creating first task\n";
    for(int l=0;l<img.height();l++){
        for(int c=0;c<img.width();c++){
            vpel = VIPS_IMAGE_ADDR(img.get_image(), c, l);// get point is too slow
            (*data)[x] = new maxtree_node((int) *vpel,x);
            t0.add_pixel(x);
            x++;
        }
    }
    std::cout << "first task created\n";
    //exit(0);
    maxtree *m = new maxtree(data, in->height(), in->width());
    threads = std::vector<std::thread*>();
    int tid;
    bool end=false;
    m->insert_component(t0.get_all_pixels_ids(), 0, t0.threshold);
    bag->insert_task(t0);
    
    for(tid = 0; tid<nth; tid++){
        threads.push_back(new std::thread(maxtree_worker,tid,bag,m));
    }
    // std::cout << "workers created " << __LINE__ << "\n" ;
    int iter=0;
    while(true){
        bag->wait_empty();
        if(bag->num_waiting() == nth && bag->empty()){
            // std::cout << "end detected" << " line: " << __LINE__ << "\n";
            bag->notify_end();
            // std::cout << "end notified: " << " line: " << __LINE__ << "\n";
            break;
        }
        // std::cout << "while true iter: " << iter++ << " line: " << __LINE__ << "\n" ;
    }
    // std::cout << "while true finished " << __LINE__ << "\n" ;

    for(auto th: threads){
        th->join();
    }

    return m;
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

    /* std::vector<int> ini = {4,1,3,2,16,9,10,14,8,7};
    max_heap<int> mh = max_heap<int>(ini);
    mh.print(); 
    
    for(auto x: {12, 30, 1, -2}){
        mh.insert(x);
    }
    mh.print(); 

    return 0; */
    
    if(argc < 3){
        std::cout << "usage:\n" << argv[0] << " <image file> <config file>\n";
        std::cout << "example:\n" << argv[0] << " input.png configs/config_test.txt\n";
        return 1;
    }
    if (VIPS_INIT(argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::unordered_map<std::string, std::string> *configs;


    
    configs = parse_config(argv[2]);
    /* test_workers();
    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    print_unordered_map(configs);
        std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n"; */
    int h,w,nth;
    bool verbose = false;

    nth = std::stoi(configs->at("threads"));
    auto conf_verbose = configs->find("verbose");
    if(conf_verbose != configs->end()){
        if(conf_verbose->second == "true"){
            verbose = true;
        }
    }

    if(argc == 4) nth = std::stoi(argv[3]);
    
    print_unordered_map(configs);
    h=in->height();
    w=in->width();
    if(verbose){
        print_VImage_band(in);
    }
    
    std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";
    
    t=maxtree_main(in,nth);

    if(verbose){    
        std::cout<<"+++++++++++++"<< __LINE__ <<"++++++++++++\n";

        for(auto thold: t->all_thresholds()){
            std::vector<component> comps = t->components_at(thold);
            std::cout << "threshold: " << thold << "\n";
            for(int i=0;i<comps.size(); i++){
                std::cout << comps[i].to_string() << "\n===================\n";
            }
        }
    }
    if(verbose){ 
        std::cout << "=====================labels=====================\n";
        std::cout << t->to_string(LABEL) << "\n================================\n";
        std::cout << "=====================parents=====================\n";
        std::cout << t->to_string();
    }
    vips_shutdown();
    return 0;
}