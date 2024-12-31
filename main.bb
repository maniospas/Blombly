add(float x, float y) => x+y+y;
print(add);


tic = time();
ret = 0;
while(i in range(1000000)) 
    ret += add(i, i);


print(time()-tic);
print(ret);