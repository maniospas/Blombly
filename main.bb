final fib = {
    res = 1;
    recur = {
        res = fib(n=n-1) + fib(n=n-2);
    }
    if(n>=1, recur);
    return(res);
}

res = fib(n=6);
print(res);