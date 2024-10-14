tic = std::time();
final fib(n) = {
    if(n<=2)
        return 1;
    return fib(n-1)+fib(n-2);
}

print(fib(20));
print(std::time()-tic);