tic = time();
ret = 0;
while(i in range(100000000)) ret += i;
print(time()-tic);
print(ret);
