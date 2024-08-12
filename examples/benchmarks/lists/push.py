import time

n = 10000000
print("-------------------------------------------")
print("Benchmark: lists wtih", n, "elements")
print("Language: Python")
print("-------------------------------------------")

x = list()
tic = time.time()
i = 0
while i<n:
    i = i+1
    x.append(i)
print("  push", time.time()-tic)

tic = time.time()
s = 0
i = 0
while i<n:
    s = s + x[i]
    i = i+1
print("  access sum", time.time()-tic)

tic = time.time()
s = 0
for xi in x:
    s = s + xi
print("  iter sum", time.time()-tic)
print(s)

print("-------------------------------------------")
print("Benchmark: lists with", n, "elements")
print("Language: Python + numpy")
print("-------------------------------------------")

tic = time.time()
s = 0
i = 0
while i<n:
    s = s + x[i]
    i = i+1
print("  access sum", time.time()-tic)

import numpy as np
x = np.array(x)
tic = time.time()
s = 0
for xi in x:
    s = s + xi
print("  iter sum", time.time()-tic)

x = np.array(x)
tic = time.time()
s = x.sum()
print("  sum", time.time()-tic)
print("")