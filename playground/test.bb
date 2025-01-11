tic = time();
ret = 0;
while(i in range(1000000)) ret += 1;
print(time()-tic);
print(ret);