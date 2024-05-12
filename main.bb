final fib = {
    if(n<2, return 1); 
    return new(n = n-1;fib:) + new(n = n-2;fib:);
}

tic = time();
print("Result", new(n=21;fib:));
print("Time", time()-tic);