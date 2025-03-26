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
        std::vector<int> pixels_index;
        int parent_thread;
        int parent_pixel;
    public:
        int size;
        int threshold;
    task(int threshold, int pt, int pp){
        parent_pixel = pp;
        parent_thread = pt;
        size = 0;
        this->threshold = threshold;
    }
    void add_pixel(int p_idx){
        size += 1;
        pixels_index.push_back(p_idx);
    }
};


template <class Task>
class bag_of_tasks{
    private:
        std::vector<Task> *tasks;
    public:
        bag_of_tasks(){
            tasks = new std::vector<Task>();
        }
        ~bag_of_tasks(){
            delete tasks;
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



std::map<int, maxtree_node*> grow_region(maxtree *m, double threshold, int ini){
    std::queue<int> queue;

}

void maxtree_worker(bag_of_tasks<task> *bag, maxtree *m, bool *end){
    task *t;
}

maxtree *maxtree_main(VImage *in, int nth = 2){
    std::vector<std::thread*> threads;
    bag_of_tasks<task> *bag;
    std::map<int, maxtree_node*> *data;
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


    //t=maxtree(in);
    //print_matrix(t, h, w);
    //label_components(t);
    //print_labels(t, h, w);
    vips_shutdown();
    return 0;
}