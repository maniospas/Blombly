def add(x, y):
    return x + y + y

print(add)

import time

tic = time.time()
ret = 0

for i in range(1000000):
    ret += i+1+1

print(time.time() - tic)
print(ret)
