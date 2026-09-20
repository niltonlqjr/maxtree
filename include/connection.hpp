#include <zmq.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "const_enum_define.hpp"

#ifndef __CONNECTION_HPP__
#define __CONNECTION_HPP__  


class handshake_monitor: public zmq::monitor_t{
private:
    std::condition_variable cv;
    std::mutex lock;
    bool handshake_done;
public:
    handshake_monitor();
    void on_event_handshake_succeeded(const zmq_event_t &event, const char* addr) override;
    void wait_handshake();
};


class connection{
    public:
        TConnectionIdx cid;    
        zmq::socket_t socket_send, socket_recv;
        
        connection(std::string addr_send, std::string addr_recv);
       
        void bind();
        void registry(zmq::context_t &ctx);

        void connect(zmq::context_t &ctx);
        void discconect();
        
    private:
        
        std::string addr_send;
        std::string addr_recv;
        bool connected, registered;
    
};

#endif