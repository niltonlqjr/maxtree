#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include <string>

#include "const_enum_define.hpp"

class message{
    private:
        enum message_type type;
        void *content;
        size_t size;
    public:
        message(enum message_type type, void *content, size_t size);
        template <class B>
        void serialize(B &buf) const;
        template <class B>
        void parse(b &buf);

            
}

#endif