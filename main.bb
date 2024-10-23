#include "libs/env"
env::include(loop);
env::include(def);

def::fn test(x,y) {
    return x+y;
}

print(test(1,2));