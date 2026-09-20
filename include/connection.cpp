#include "connection.hpp"
#include <iostream>
handshake_monitor::handshake_monitor(){
    this->handshake_done = false;
}

void handshake_monitor::on_event_handshake_succeeded(const zmq_event_t &event, const char* addr) {
    std::unique_lock<std::mutex> l(this->lock);
    this->handshake_done = true;
    this->cv.notify_all();
}

void handshake_monitor::wait_handshake(){
    std::unique_lock<std::mutex> l(this->lock);
    if(!handshake_done){
        this->cv.wait(l);
    }
}

connection::connection(std::string addr_send, std::string addr_recv){
    this->addr_send = addr_send;
    this->addr_recv = addr_recv;
    this->connected = false;
    this->registered = false;

}

void connection::bind(){
    this->socket_send.bind(this->addr_send);
    this->socket_recv.bind(this->addr_recv);
}

void connection::registry(zmq::context_t &ctx){
    // std::string msg_content = hps::to_string(*this);
    
    // std::string s_msg = hps::to_string(requirement);
    std::string s_msg;
    std::string str, _m;
    zmq::message_t message_0mq(s_msg);
    zmq::message_t reply_0mq;
    
    this->socket_send = zmq::socket_t(ctx, zmq::socket_type::dealer);
    this->socket_recv = zmq::socket_t(ctx, zmq::socket_type::dealer);
    
    this->socket_recv.connect(this->addr_recv);
    this->socket_recv.send(message_0mq, zmq::send_flags::none);

    #ifdef VERBOSE
        _m = "send " + std::to_string(this->id) + "\n";
        std::cout << _m;
    #endif
    
    auto resp_val = this->socket_recv.recv(reply_0mq, zmq::recv_flags::none);
    
    #ifdef VERBOSE
        _m = "recv " + std::to_string(this->id) + " - registration successful\n";
        std::cout << _m;
    #endif
    
    TConnectionIdx new_idx = std::stoi(reply_0mq.to_string());
    this->cid = new_idx;
    #ifdef VERBOSE
        str+="new id: " +std::to_string(this->get_index());
        std::cout << str + "\n";
    #endif
    this->registered = true;
    this->socket_recv.disconnect(this->addr_recv);
}


void check_event(handshake_monitor &hsmonitor){
    hsmonitor.check_event(-1);
}

void connection::connect(zmq::context_t &ctx){
    std::string _m;
    if(!this->connected){
        if(this->registered){
            this->socket_send.set(zmq::sockopt::routing_id, std::to_string(this->cid));
            this->socket_recv.set(zmq::sockopt::routing_id, std::to_string(this->cid));
        }else{
            _m = "connection: " + std::to_string(this->cid) + " not registered at server " + this->addr_recv + " \n";
            std::cerr << _m;
        }
        handshake_monitor monitor_send, monitor_recv;
        
        std::string monitor_send_name = "inproc://monitor_send" + std::to_string(this->cid);
        monitor_send.init(this->socket_send, monitor_send_name, ZMQ_EVENT_HANDSHAKE_SUCCEEDED);
        this->socket_send.set(zmq::sockopt::linger, 0);
        std::thread monitor_thread_send(check_event, std::ref(monitor_send));
        this->socket_send.connect(this->addr_send);
        monitor_thread_send.join();

        #ifdef VERBOSE
            _m = std::to_string(this->get_index()) + "connected socket_send at: " + this->addr_send + "\n";
            std::cout << _m;
        #endif

        std::string monitor_recv_name = "inproc://monitor_recv" + std::to_string(this->cid);
        monitor_recv.init(this->socket_recv, monitor_recv_name, ZMQ_EVENT_HANDSHAKE_SUCCEEDED);
        this->socket_recv.set(zmq::sockopt::linger, 0);
        std::thread monitor_thread_recv(check_event, std::ref(monitor_recv));
        this->socket_recv.connect(this->addr_recv);
        monitor_thread_recv.join();
        
        #ifdef VERBOSE
            _m = std::to_string(this->get_index()) + "connected socket_recv at: " + this->manager_recv + "\n";
            std::cout << _m;
        #endif
    }
    this->connected = true;

}

void connection::discconect(){
    std::string _m;
    if(this->connected){
        this->socket_recv.disconnect(this->addr_recv);
        this->socket_send.disconnect(this->addr_send);
        #ifdef VERBOSE
            _m = std::to_string(this->id) + " disconnected from " + this->addr_send + " and " + this->addr_recv +  "\n";
            std::cout << _m;
        #endif
    }
    this->connected = false;
}
