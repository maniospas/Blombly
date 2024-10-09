from time import time

tic = time()
def fib(n):
    if(n<=2):
        return 1
    return fib(n-1)+fib(n-2)

print(fib(32))
print(time()-tic)