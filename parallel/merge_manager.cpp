#include <string>
#include <thread>

#include <zmq.hpp>

#include "utils.hpp"
#include "workers.hpp"
#include "scheduler_of_workers.hpp"
#include "const_enum_define.hpp"
#include "boundary_tree.hpp"
#include "message.hpp"
#include "src/hps.h"

using namespace vips;

uint32_t glines, gcolumns;

bool print_only_trees;
bool verbose;

bool running;

static const int nth = 2;

TWorkerIdx idx_at_manager=0;

scheduler_of_workers<worker*> system_workers;

void read_config(char conf_name[], std::string &port, std::string &protocol);




bool reply_to(std::string ip, worker *w){
    zmq::context_t context(1);
    zmq::socket_t rep(context, zmq::socket_type::push);
    rep.connect(ip);
    std::string s_newid = hps::to_string<TWorkerIdx>(w->get_index());
    zmq::message_t msg_newid(s_newid);
    std::optional<size_t> send_result = rep.send(msg_newid,zmq::send_flags::none);
    if(send_result.has_value()){
        rep.disconnect(ip);
        return true;
    }
    return false;
}

void read_config(char conf_name[], std::string &port, std::string &protocol){

    auto configs = parse_config(conf_name);

    if(configs->find("glines") == configs->end() || configs->find("gcolumns") == configs->end()){
        std::cout << "you must specify the the image division on config file:" << conf_name <<"\n";
        std::cout << "example:\n";
        std::cout << "glines=8 #divide image in 8 vertical tiles\n";
        std::cout << "gcolumns=6 #divide image in 6 vertical tiles\n";
        exit(EX_CONFIG);
    }
    glines = std::stoi(configs->at("glines"));
    gcolumns = std::stoi(configs->at("gcolumns"));

    port = get_field(configs, "port", DEFAULT_PORT);
    protocol = get_field(configs, "protocol", "tcp");
}


std::pair<uint32_t, uint32_t> next_tile(std::pair<uint32_t, uint32_t> p){
    std::pair<uint32_t, uint32_t> newp;
    newp.second = (p.second + 1) % GRID_DIMS.second;
    newp.first = p.first + (newp.second != p.second+1);
    return newp;
}

void manager_recv(scheduler_of_workers<worker *> *pool_of_workers, zmq::socket_t &sock){

    // todos os trabalhadores devem procurar um balanceamento na equação 1.
    //      trabalho_realizado = poder_de_processamento * 1/peso_das_tarefas_realizadas (1)
    // sendo assim, todas solicitações vão procurar responder com o trabalhador que tem
    // o menor coeficiente de trabalho_realizado dentre os disponíveis.
    // é uma estratégia gulosa.

    zmq::message_t request;
    std::pair<uint32_t, uint32_t> current_tile(0,0);
    
    TWorkerIdx current_idx;

    while (running){
        std::cout << "------>new iteration\n";
        auto res_recv = sock.recv(request);
        
        std::string rec_msg;
        rec_msg.resize(request.size());
        memcpy(rec_msg.data(), request.data(), request.size());
        // std::cout << "receieved:" << rec << "\n";
        std::cout << "receieved: --> ";
        for(char c: rec_msg){
            std::cout << " " << (int) c;
        }
        std::cout << " <--\n";
        message recv_msg = hps::from_string<message>(rec_msg);
        std::cout << "message type" << recv_msg.type << "\n";
        if(recv_msg.type == MSG_REGISTRY){
            worker w_rec = hps::from_string<worker>(recv_msg.content);
            std::cout << "Registry" << "\n";
            w_rec.print();
            current_idx=idx_at_manager++;
            w_rec.update_index(current_idx);
            worker *w_at_manager = new worker(w_rec);
            system_workers.insert_worker(w_at_manager);
            std::cout << "========================== registered workers: ==========================\n";
            for(size_t i=0; i < system_workers.size(); i++){
                worker *w = system_workers.at(i);
                w->print();
            }
            std::cout << "==========================  ==========================\n";
            
            zmq::message_t msg_new_id(sizeof(TWorkerIdx));
            memcpy(msg_new_id.data(), &current_idx, sizeof(TWorkerIdx));
            
            auto reply_return = sock.send(msg_new_id, zmq::send_flags::none);

        }else if(recv_msg.type == MSG_BOUNDARY_TREE){
            std::cout << "tree" << "\n";
            boundary_tree bt = hps::from_string<boundary_tree>(recv_msg.content);
            std::cout << "tree ok\n";
            std::cout << "-------------------------- Boundary Tree ----------------------------\n";
            bt.print_tree();
            auto reply_return = sock.send(zmq::buffer("TREE_RECV"), zmq::send_flags::none);
        }else if(recv_msg.type == MSG_GET_TILE){
            
            TWorkerIdx worker_idx = hps::from_string<TWorkerIdx>(recv_msg.content);
            std::pair<uint32_t,uint32_t> reply;

            if(!inside_rectangle(current_tile, GRID_DIMS)){
                reply = GRID_DIMS;
            }else{
                reply = current_tile;
            }
            current_tile = next_tile(reply);
            std::cout << "Current tile: (" << current_tile.first << "," << current_tile.second << ")\n";
            std::cout << "Reply Tile: (" << reply.first << "," << reply.second << ")\n";
            // std::string s_reply = hps::to_string<std::pair<uint32_t,uint32_t>>(reply);
            std::string s_reply = hps::to_string(reply);
            zmq::message_t msg_grid(s_reply);
            sock.send(msg_grid, zmq::send_flags::none);

        }else{
            std::cout << "invalid message received from " << recv_msg.sender << "\n";
            running = false;
            sock.send(zmq::buffer("FAIL"), zmq::send_flags::none);
        }
        std::cout << "end iteration<--------\n";
    }
}


int main(int argc, char *argv[]){
    scheduler_of_workers<worker *> *pool_of_workers;
    std::string port, protocol;

    read_config(argv[1], port, protocol);
    GRID_DIMS = std::make_pair(glines,gcolumns);
    //  Prepare contexts and sockets
    
    zmq::context_t context_rec(nth);
    zmq::context_t context_reg(nth);
    zmq::socket_t  receiver_sock(context_rec, zmq::socket_type::pull);
    zmq::socket_t  registry_sock(context_reg, zmq::socket_type::rep);

    
    

    std::string address = protocol+"://*:"+port;
    registry_sock.bind(address);
    
    running=true;
    std::thread *r = new std::thread(manager_recv, std::ref(pool_of_workers), std::ref(registry_sock));

    r->join();
    
}