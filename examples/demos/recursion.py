from time import time

def fib(n):
    if n<2:
        return 1
    return fib(n-1) + fib(n-2)

tic = time()
print("Result", fib(n=18))
print("Time", time()-tic)