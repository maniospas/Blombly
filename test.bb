#include "libs/loop"

while(i as loop::range(5)) {
    //
    while(j as loop::range(5))
        print(i, j);
}


#include "libs/dict"

x = dict();
x["hi"] = 1;
x[2] = 3;
x[2] = 4;
it = iter(x);
while(i as next(it))
    print(i, x[i]);