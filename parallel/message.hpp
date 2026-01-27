#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include <string>

#include "src/hps.h"
#include "const_enum_define.hpp"

class message{
    
    public:
        std::string content;
        size_t size;
        enum message_type type;
        
        message();
        message(std::string &content, size_t size=0, enum message_type type=MSG_NULL);

        template <class B>
        void serialize(B &buf) const{
            buf << (unsigned int) this->type << this->size << this->content;
        }


        template <class B>
        void parse(B &buf){
            buf >> (unsigned int&) this->type >> this->size >> this->content;
        }

            
};

#endif