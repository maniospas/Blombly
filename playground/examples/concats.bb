tic = time();

result = "";
while(i in range(100000)) result += "abc";

print(result|len, result[result|len-1]);

print(time()-tic);