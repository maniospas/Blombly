import time

tic = time.time()
ret = 0

for i in range(100000000):
    ret += i

print(time.time() - tic)
print(ret)
