#include <cstring>
#include "message.hpp"

message::message(){
    this->content = "";
    this->size = 0;
    this->type = MSG_NULL;
    this->sender = 0;
}

message::message(std::string &content, size_t size, enum message_type type, TWorkerIdx sender){
    this->type = type;
    this->size = size;
    this->content = content;
    this->sender = sender;

}
