from datetime import datetime
from random import randint

arr = [randint(-1000,1000) for i in range(10000)]

def bruteForce(a):
  l = len(a)
  max = 0
  for i in range(l):
    sum = 0
    for j in range(i, l):
      sum += a[j]
      if(sum > max):
        max = sum
  return max

start = datetime.now()
bruteForce(arr)
end = datetime.now()

print(format(end-start))
