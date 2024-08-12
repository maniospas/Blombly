import time

def factorial(n):
    if n == 1:
        return 1
    return n * factorial(n-1)

tic = time.time()
result = factorial(100)
toc = time.time()
print("Factorial of 100: ", result)
print("Time taken: ", toc-tic)