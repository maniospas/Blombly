final fib = {
    if(n<2, return 1); 
    return fib(n=n-1)+fib(n=n-2);
}

print(fib(n=9)); // 55