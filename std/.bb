#include "std\oop"
#include "std\fmt"
#include "std\loop"

#macro (use std;) = ( 
    // use oop
    #macro (fn) = (std::fn); 
    #macro (module) = (std::module); 
    #macro (abstract) = (std::abstract); 
    // use fmt
    #macro (printfm) = (std::print); 
    // allow explicit reference still
    #macro (std::std) = (std);
);
#macro (use @name;) = (#fail "Invalid syntax.\n   !!! The statement `use @namespace;` has been reserved\n       for the simplifation of specific namespaces." @next;);
#macro (std @next) = (#fail "Unable to find a matching std macro." @next;);