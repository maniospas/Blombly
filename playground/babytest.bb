tic = time();

final fact(n) = {
    if(n==0) {return 1;}
    return n*fact(n-1);
}

x = fact(20);
print("here");
print(x);
print(time()-tic);