tic = time();

final fib(n) = {
    if(n<=1) return 1;
    return fib(n-1)+fib(n-2);
}
x = fib(32);
print("running");
print(x);
print(time()-tic);