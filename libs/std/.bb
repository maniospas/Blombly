#include "libs\std\oop"

#macro (enable std::convert;) = { 
    // these are builtin type convertion symbols
    #macro (int)   = {std::int}
    #macro (float) = {std::float}
    #macro (str)   = {std::str}
}

#macro (enable std::list;) = {
    // builtin symbols (of the bbvm namespace) to work with lists
    #macro (list) = {std::list}
    #macro (map)  = {std::map}
    #macro (push) = {std::push}
    #macro (pop)  = {std::pop}
    #macro (len)  = {std::len}
    #macro (next) = {std::next}
    #macro (iter) = {std::iter}
}

#macro (enable std::vector;) = {
    // builtin symbols (of the bbvm namespace) to work with vectors
    #macro (vector) = {std::vector}
    #macro (len) = {std::len}
    #macro (iter) = {std::iter}
}

#macro (enable std::oop;) = {
    #macro (fn) = {std::fn}
    #macro (module) = {std::module}
    #macro (abstract) = {std::abstract}
}

#macro (enable std;) = {
    #macro (print) = {std::print}
    #macro (read) = {std::read}
    enable std::convert;
    enable std::list;
    enable std::vector;
}