#include "std/oop"

fn fib(n) {
    #spec atomic = false;
    if(n<2)
      return 1;
    //return fib(n=n-1) + fib(n=n-2);
    return fib(n-1) + fib(n-2);
}

tic = time();
result = fib(21);
print("Result " + str(result));
print("Time " + str(time()-tic));