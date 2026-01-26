#include <string>
#include <zmq.hpp>

#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
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

void manager_recv(scheduler_of_workers<worker *> *pool_of_workers, zmq::socket_t &s){
    zmq::message_t request;
    while (true){
        s.recv(request, zmq::recv_flags::none);
        std::string rec;
        rec.resize(request.size());
        memcpy(rec.data(), request.data(), request.size());
        std::cout << "receieved:" << rec << "\n";
        message r = hps::from_string<message>(rec);
        std::cout << "message type" << r.type << "\n";
        if(r.type == MSG_REGISTRY){
            worker w = hps::from_string<worker>(r.content);
            w.print();
            system_workers.insert_worker(new worker(w));
        }

        constexpr std::string_view reply_string = "OK";
        zmq::message_t reply (reply_string.length());
        memcpy (reply.data (), reply_string.data(), reply_string.length());
        s.send (reply, zmq::send_flags::none);
        std::cout << "registered workers:\n";
        for(size_t i=0; i < system_workers.size(); i++){
            worker *w = system_workers.at(i);
            w->print();
        }
    }
}


int main(int argc, char *argv[]){
    scheduler_of_workers<worker *> *pool_of_workers;
    std::string port, protocol;

    read_config(argv[1], port, protocol);

    //  Prepare our context and socket
    static const int nth = 2;
    zmq::context_t context (nth);
    zmq::socket_t socket (context, zmq::socket_type::rep);
    std::string address = protocol+"://*:"+port;
    socket.bind(address);
    manager_recv(pool_of_workers, socket);


}