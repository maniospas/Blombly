final fib = {
    if(n<2)
        return 1;
    return fib(n=n-1)+fib(n=n-2);
}

tic = std::time();
std::print("Result", fib(n=21));
std::print("Time", std::time()-tic);