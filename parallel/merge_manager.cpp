#include <string>
#include <zmq.hpp>

#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
#include "message.hpp"

void read_config(char conf_name[], std::string &port, std::string &protocol);

void read_config(char conf_name[], std::string &port, std::string &protocol){

    auto config = parse_config(conf_name);
    port="7233"; 
    if(config->find("port") != config->end()){
        port = config->at("port");
    }
    protocol="tcp";
    if(config->find("protocol") != config->end()){
        protocol = config->at("protocol");
    }
}

void manager_recv(scheduler_of_workers<worker *> *pool_of_workers, zmq::socket_t &s){
    
}


int main(int argc, char *argv[]){
    scheduler_of_workers<worker *> *pool_of_workers;
    std::string port, protocol;


    auto config = read_config(argv[1], port, protocol);

    //  Prepare our context and socket
    static const int nth = 2;
    zmq::context_t context (nth);
    zmq::socket_t socket (context, zmq::socket_type::rep);
    std::string address = protocol+"://*"+port;
    socket.bind(address);
    zmq::message_t request;
    s.recv(request);


}