tic = time();
ret = 0;
while(i in range(1000000)) ret += i;
print(time()-tic);
print(ret);