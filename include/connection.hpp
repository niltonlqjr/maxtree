#include <zmq.hpp>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "const_enum_define.hpp"

#ifndef __CONNECTION_HPP__
#define __CONNECTION_HPP__  





class connection{
    public:
        TConnectionIdx cid;    
        zmq::socket_t socket_send, socket_recv;
        connection();
        connection(zmq::context_t &ctx, std::string addr_send, std::string addr_recv,  zmq::socket_type type);

        void prepare_sockets(zmq::context_t &ctx, std::string addr_send, std::string addr_recv, zmq::socket_type type);
        void bind();
        void registry();

        void connect();
            
        void disconnect();

        void send_message(std::string id, std::string msg);
        std::pair<std::string, std::string> recv_message();
    
        class handshake_monitor: public zmq::monitor_t{
            private:
                std::condition_variable cv;
                std::mutex lock;
                bool handshake_done;
                std::string monitor_name;
                std::thread thread_monitor;
                void func_check_event();
            public:
                handshake_monitor(std::string name);
                void on_event_handshake_succeeded(const zmq_event_t &event, const char* addr) override;
                void wait_handshake();
                void event(zmq::socket_t &socket);
                void join_event();
        }; 
    private:
        std::string addr_send;
        std::string addr_recv;
        bool connected, registered;
        zmq::socket_type socket_type;
    
};

#endif