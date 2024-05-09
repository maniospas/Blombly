final fib = {
    if(n<2, return 1); 
    return fib(n=n-1)+fib(n=n-2);
}

tic = time();
print("Result", fib(n=18));
print("Time", time()-tic);