import numpy as np
import time

def fib(n):
    f = np.zeros(n)
    f[0] = 1
    f[1] = 1
    i = 2
    while(i<n):
      f[i] = f[i-1] + f[i-2]
      i = i + 1
    return f[n-1]

tic = time.time()
result = fib(21)
print(time.time()-tic)
print("Result " + str(result))