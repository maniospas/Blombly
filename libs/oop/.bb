#include "libs/oop/fn"
#include "libs/oop/class"

// enables the 
#macro (enable oop;) = {
    #macro (fn) = {oop::fn}
    #macro (module) = {oop::module}
    #macro (abstract) = {oop::abstract}
    #macro (uses) = {oop::uses}
    #macro (class) = {oop::class}
}