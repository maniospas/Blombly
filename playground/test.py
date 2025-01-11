import time

tic = time.time()
ret = 0

for i in range(1000000):
    ret += 1

print(time.time() - tic)
print(ret)
