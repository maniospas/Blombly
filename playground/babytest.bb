tic = time();

final fact(n) = {
    if(n==0) return 1;
    return n*fact(n-1);
}

print(fact(20));
print(time()-tic);