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
    while (true){
        std::cout << "------>new iteration\n";
        r.recv(request, zmq::recv_flags::none);
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
            worker w = hps::from_string<worker>(r.content);
            std::cout << "Registry" << "\n";
            w.print();
            system_workers.insert_worker(new worker(w));
            std::cout << "========================== registered workers: ==========================\n";
            for(size_t i=0; i < system_workers.size(); i++){
                worker *w = system_workers.at(i);
                w->print();
            }
        }else if(r.type == MSG_BOUNDARY_TREE){
            std::cout << "tree" << "\n";
            boundary_tree bt = hps::from_string<boundary_tree>(r.content);
            std::cout << "tree ok\n";
            // bt.print_tree();
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