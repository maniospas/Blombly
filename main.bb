#include "libs/loop"
#include "libs/symb"
A = 15, 1, 2;
A = loop::tolist(A|x->try if(x<10) return x);
print(A);