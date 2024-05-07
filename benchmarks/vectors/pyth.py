import numpy as np
import time

n = 1000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
tic = time.time()
z = np.zeros((n,))
i = 0
#z = x+y
while i<n:
    z[i] = x[i]+y[i]
    i = i+1
toc = time.time()
print(toc-tic)