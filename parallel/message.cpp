#include <cstring>
#include "message.hpp"
#include "hps.h"

message::message(enum message_type type, void *content, size_t size){
    this->type = type;
    this->size = size;
    this->content = new uint8_t[size];
    std::memcpy(this->content, content, size);
}

void message::serialize(B &buf) const {
    buf << this->type << this->size;
    uint8_t *content_ptr = (uint8_t *) this->content;
    for(i=0;i<size;i++){
        buf << *content_ptr;
        content_ptr++;
    }
}

void message::parse(B &buf){
    buf >> this->type >> this->size;
    uint8_t *content_ptr = (uint8_t *) this->content;
    for(i=0;i<this->size;i++){
        buf >> *content_ptr;
        content_ptr++;
    }
}