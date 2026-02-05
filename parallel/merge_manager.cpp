#include <string>
#include <zmq.hpp>

#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
#include "boundary_tree.hpp"
#include "message.hpp"
#include "src/hps.h"

using namespace vips;
bool print_only_trees;
bool verbose;

TWorkerIdx idx_at_manager=0;

scheduler_of_workers<worker*> system_workers;

void read_config(char conf_name[], std::string &port, std::string &protocol);

void read_config(char conf_name[], std::string &port, std::string &protocol){

    std::unordered_map<std::string, std::string> *config = parse_config(conf_name);
    port=DEFAULT_PORT; 
    if(config->find("port") != config->end()){
        port = config->at("port");
    }
    protocol="tcp";
    if(config->find("protocol") != config->end()){
        protocol = config->at("protocol");
    }
}

void manager_recv(scheduler_of_workers<worker *> *pool_of_workers, zmq::socket_t &r){
    zmq::message_t request;
    std::pair<uint32_t, uint32_t> grid_idx;
    while (true){
        std::cout << "------>new iteration\n";
        auto res_recv = r.recv(request);
        
        std::string rec;
        rec.resize(request.size());
        memcpy(rec.data(), request.data(), request.size());
        // std::cout << "receieved:" << rec << "\n";
        std::cout << "receieved: --> ";
        for(char c: rec){
            std::cout << " " << (int) c;
        }
        std::cout << " <--\n";
        message r = hps::from_string<message>(rec);
        std::cout << "message type" << r.type << "\n";
        if(r.type == MSG_REGISTRY){
            worker w_rec = hps::from_string<worker>(r.content);
            std::cout << "Registry" << "\n";
            w_rec.print();
            w_rec.update_index(idx_at_manager++);
            worker *w_at_manager = new worker(w_rec);
            system_workers.insert_worker(w_at_manager);
            std::cout << "========================== registered workers: ==========================\n";
            for(size_t i=0; i < system_workers.size(); i++){
                worker *w = system_workers.at(i);
                w->print();
            }

        }else if(r.type == MSG_BOUNDARY_TREE){
            std::cout << "tree" << "\n";
            boundary_tree bt = hps::from_string<boundary_tree>(r.content);
            std::cout << "tree ok\n";
            std::cout << "-------------------------- Boundary Tree ----------------------------\n";
            bt.print_tree();
        }else if(r.type == MSG_GET_GRID){
            // grid_idx = get_grid_for_worker()

        }
        std::cout << "end iteration<--------\n";
    }
}


int main(int argc, char *argv[]){
    scheduler_of_workers<worker *> *pool_of_workers;
    std::string port, protocol;

    read_config(argv[1], port, protocol);

    //  Prepare our context and socket
    static const int nth = 2;
    zmq::context_t context (nth);
    zmq::socket_t  receiver(context, zmq::socket_type::pull);
    std::string address = protocol+"://*:"+port;
    receiver.bind(address);
    manager_recv(pool_of_workers, receiver);


}