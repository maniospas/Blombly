final fib(n) = {  
    if (n < 2) return 1;  
    return fib(n-1) + fib(n-2);
}

tic = time();
result = fib(31);
toc = time();
print("Result !{result}");
print("Elapsed !{toc-tic} sec");
