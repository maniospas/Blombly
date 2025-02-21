tic = time();

text = "I like banana";
applied = bb.string.starts("banana");
while(i in range(300000)) result = text|applied;

print(result);

print(time()-tic);