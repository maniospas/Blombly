#include "libs\std\oop"

#macro (enable std::converters;) = { 
    // these are builtin type convertion symbols
    #macro (int)   = {std::int}
    #macro (float) = {std::float}
    #macro (str)   = {std::str}
}

#macro (enable std::lists;) = {
    // builtin symbols (of the bbvm namespace) to work with lists
    #macro (list) = {std::list}
    #macro (map)  = {std::map}
    #macro (push) = {std::push}
    #macro (pop)  = {std::pop}
    #macro (len)  = {std::len}
    #macro (next) = {std::next}
    #macro (iter) = {std::iter}
}

#macro (enable std::vectors;) = {
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
    enable std::converters;
    enable std::lists;
    enable std::vectors;
    enable std::oop;
}