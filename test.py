import time

buff = 0
tic = time.time()
for i in range(100000):
    buff += i
toc = time.time()

print((buff))
print((toc - tic), "sec")
