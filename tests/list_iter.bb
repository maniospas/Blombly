#include "libs/env"
env::include(loop);

A = 1,2,3;
sum = 0;
while(x as loop::next(A)) { 
    sum += x;
}

it = iter(A);
while(x as std::next(it)) {
    sum -= x;
}

if(sum!=0)
    fail("Wrong operations");
print(sum);