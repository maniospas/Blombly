tic = time();
ret = 0;
n = 100000000;
while(i in range(n)) ret += i;
print(time()-tic);
print(ret);
