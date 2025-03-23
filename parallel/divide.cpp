#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vips/vips8>
#include <vector>
#include <algorithm>
#include <thread>
#include <queue>
#include "maxtree_node.hpp"
#include "utils.hpp"

using namespace vips;

typedef enum task_type {PROCESS, MERGE};

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

void maxtree_worker(bag_of_tasks<task> *bag, std::map<int, maxtree_node*> *data, bool *end){
    task *t;
}

std::vector<maxtree_node*> *maxtree_main(VImage *in, int nth = 2){
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
int main(int argc, char **argv){
    VImage *in;
    std::map<int, maxtree_node*> *t;
    if (VIPS_INIT (argv[0])) 
        vips_error_exit (NULL);

    in = new VImage(VImage::new_from_file(argv[1],NULL));

    std::map<std::string, std::string> *configs;

    configs = parse_config(argv[2]);
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