import numpy as np
import time

n = 1000000
x = np.zeros((n,))
y = np.zeros((n,))
tic = time.time()
z = np.zeros((n,))
i = 0
while i<n:
    z[i] = x[i]+y[i]
    i = i+1
toc = time.time()
print(toc-tic)