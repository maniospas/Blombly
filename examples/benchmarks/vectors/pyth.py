import numpy as np
import time

n = 10000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
tic = time.time()
#z = x+y
z = np.zeros((n,))
for i in range(n):
    temp = x[i]+y[i]
toc = time.time()
print(toc-tic)