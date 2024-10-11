#include "libs/env" // includes the advanced documentation and dependency library to substitute include statements
env::include("loop");
env::versions();

A = 1,2,3;
while(x as loop::next(A)) 
    print(x);