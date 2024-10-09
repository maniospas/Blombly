from time import time

tic = time()
x = list()
for i in range(1000000):
    x.append(i)

print(time()-tic)