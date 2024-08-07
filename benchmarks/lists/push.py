import time

x = list()
tic = time.time()
i = 0
while i<1000000:
    i = i+1
    x.append(i)
print(time.time()-tic)

tic = time.time()
s = 0
i = 0
while i<1000000:
    s = s + x[i]
    i = i+1
print(time.time()-tic)

tic = time.time()
for xi in x:
    s = s + xi
print(time.time()-tic)
