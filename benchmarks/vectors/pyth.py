import numpy as np
import time

n = 1000000
x = np.zeros((n,))+0.1
y = np.zeros((n,))+0.1
z = np.zeros((n,))
tic = time.time()
i = 0
#z = x+y
#while i<n:
    #if i<1:
    #    print("test")
    #z[i] = x[i]+y[i]
    #i = i+1
s = 0
for i in x:
    s = s+i
toc = time.time()
print(toc-tic)