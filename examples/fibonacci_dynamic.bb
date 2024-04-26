final fib = {
    n = n+1;
    res = Vector(n);
    res[0] = 1;
    res[1] = 1;
    i = 2;
    while(i<n,
        f = res[i-2]+res[i-1];
        res[i] = f;
        i = i+1;
    );
    return(res[n-1]);
}

print(fib(n=9)); // 55