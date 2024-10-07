#include "libs\std\oop"

#macro (enable std::web;) = { 
    // these are builtin web service tools
    #macro (server) = {std::server}
}

#macro (enable std;) = {
    enable std::web;
    enable std::oop;
}