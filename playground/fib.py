def fib(n):
    if n < 2:
        return 1
    return fib(n - 1) + fib(n - 2)

import time

tic = time.time()
result = fib(31)
print("Result", result)
print("Time", time.time() - tic)
