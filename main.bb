#include "libs/std"
enable std;

n = 1000000;

x = list(1);
tic = time();
i = 0;
while(i<n){
    i = i+1;
    push(x, i);
}

print(x[0]);