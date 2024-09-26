final fib = {
    if(n<2)
        return 1;
    return new{n = n-1;fib:} + new{n = n-2;fib:}
}

tic = std::time();
std::print("Result", new{n=21;fib:});
std::print("Time", std::time()-tic);