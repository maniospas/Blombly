final fib = {
    i = 0; while(i<1000000, i=i+1); // create delays to check that synchronization works
    if(n<2, return 1); 
    return fib(n=n-1)+fib(n=n-2);
}

tic = time();
print("Result", fib(n=12));
print("Time", time()-tic);