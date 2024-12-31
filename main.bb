add(int x, int y) => x+y*2;
print(add);

tic = time();
ret = 0;
while(i in range(1000000)) 
    ret += add(i, 1);

print(time()-tic);
print(ret);