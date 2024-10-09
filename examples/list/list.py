from time import time

tic = time()
x = list()
i = 0
while(i<100000):
    x.append(i)
    i = i+1

print(time()-tic)