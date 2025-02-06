tic = time();

text = "I like banana";
applied = bb.string.starts("I like");
while(i in range(300000)) result = text|applied;

print(result);

print(time()-tic);