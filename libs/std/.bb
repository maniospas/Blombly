#include "libs\std\oop"

#macro (enable std::convert;) = { 
    // these are builtin type convertion symbols
    #macro (int)   = {std::int}
    #macro (float) = {std::float}
    #macro (str)   = {std::str}
}

#macro (enable std::collect;) = {
    // builtin symbols (of the bbvm namespace) to work with lists
    #macro (list) = {std::list}
    #macro (map)  = {std::map}
    #macro (push) = {std::push}
    #macro (pop)  = {std::pop}
    #macro (len)  = {std::len}
    #macro (next) = {std::next}
    #macro (iter) = {std::iter}
}

#macro (enable std::linear;) = {
    // builtin symbols (of the bbvm namespace) to work with vectors
    #macro (vector) = {std::vector}
    #macro (len) = {std::len}
    #macro (iter) = {std::iter}
}

#macro (enable std::web;) = { 
    // these are builtin web service tools
    #macro (server)   = {std::server}
}

#macro (enable std::console;) = { 
    // these are builtin console tools
    #macro (print) = {std::print}
    #macro (read) = {std::read}
    #macro (time) = {std::time}
}

#macro (enable std;) = {
    enable std::console;
    enable std::convert;
    enable std::collect;
    enable std::linear;
    enable std::web;
    enable std::oop;
}