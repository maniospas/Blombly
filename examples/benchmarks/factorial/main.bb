final factorial = {
    if(n==1)
    		return 1.0;
    return n*new{n=n-1;factorial:}
}

tic = time();
f = factorial(n=100);
print(f);
print(time()-tic);