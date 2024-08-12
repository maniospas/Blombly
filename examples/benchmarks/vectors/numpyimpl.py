import numpy as np
import time

n = 10000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
print(y.dtype)
tic = time.time()
z = x+y
toc = time.time()
print(toc-tic)