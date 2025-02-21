import time
tic = time.time()
text = "I like banana"
for i in range(300000):
    result = text.startswith("banana")
print(result)
print(time.time() - tic)