tic = time();
text = "I like banana";
while(i in range(300000))
    result = text|bb.string.starts("banana");
print(result);
print(time() - tic);