#include <cstring>
#include "message.hpp"

message::message(){
    this->content = "";
    this->size = 0;
    this->type = MSG_NULL;
    this->sender = "";
}

message::message(std::string &content, size_t size, enum message_type type, std::string sender){
    this->type = type;
    this->size = size;
    this->content = content;
    this->sender = sender;

}
