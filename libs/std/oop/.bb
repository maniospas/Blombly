#include "libs/std/oop/fn"
#include "libs/std/oop/class"

// enables the 
#macro (enable std::oop;) = {
    #macro (fn) = {std::fn}
    #macro (module) = {std::module}
    #macro (abstract) = {std::abstract}
    #macro (uses) = {std::uses}
    #macro (class) = {std::class}
}