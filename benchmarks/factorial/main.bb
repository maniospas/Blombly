final factorial = {
    if(n==1, return 1.0);
    return n*factorial(n=n-1);
}

tic = time();
f = factorial(n=100);
f = f;
print(time()-tic);