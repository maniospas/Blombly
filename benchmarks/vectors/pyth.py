import numpy as np
import time

n = 10000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
z = np.zeros((n,))
tic = time.time()
i = 0
#z = x+y
while i<n:
    z[i] = x[i]+y[i]
    i = i+1
toc = time.time()
print(toc-tic)