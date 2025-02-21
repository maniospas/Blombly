import time
tic = time.time()
text = ""
for i in range(100000):
    text += "abc"
print(len(text))
print(time.time() - tic)