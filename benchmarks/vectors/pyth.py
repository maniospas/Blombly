import numpy as np
import time

n = 1000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
z = np.zeros((n,))
tic = time.time()
#z = x+y
#for i in range(n):
    #temp = x[i]+y[i]
    #if temp>0:
    #    z[i] = temp
i = 0
while i < n:
    i = i+1
    j = 1
    if j>0:
        j=0
toc = time.time()
print(toc-tic)