#include "std\oop"
#include "std\fmt"
#include "std\loop"

#macro (std::simplify;) = ( 
    #macro (fn) = (std::fn); 
    #macro (module) = (std::module); 
    #macro (abstract) = (std::abstract); 
    #macro (printfm) = (std::print); 
);
#macro (std @next) = (#fail "Unable to find a matching std macro." @next;);