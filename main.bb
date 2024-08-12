#include "std/oop"
final fibber = new {
  fn fib(n) {
      while(false){}
      if(n<2)
        return 1;
      return fib(n-1) + fib(n-2);
  }
}

tic = time();
result = fibber.fib(21);
print("Result " + str(result));
print("Time " + str(time()-tic));