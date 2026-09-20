#include "connection.hpp"
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
}

void connection::connect(zmq::context_t &ctx){
}

void connection::discconect(){
}
