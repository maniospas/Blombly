#include "libs/final"
#include "libs/loop"

final::def functors(n) {
    ret = list();
    sqrtn = int(n^0.5+1);
    while(i as loop::range(1, sqrtn)) {
        if(n%i==0) {
            push(ret, i);
            complement = int(n/i);
            if(i!=complement)
                push(ret, complement);
        }
    }
    return ret;
}


print(functors(100));