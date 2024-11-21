#include "libs/env"
env::include(loop);

A = 1,2,3;
s = 0;
while(x as loop::next(A)) 
    s += x;

it = iter(A);
while(x as bbvm::next(it))
    s -= x;

if(s!=0)
    fail("Wrong operations");
print(s);