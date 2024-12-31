add(float x, float y) => x+y+x+y+x+y*2;

tic = time();
ret = 0;
while(i in range(1000000))
    ret += add(i, 1);


print(time()-tic);
print(ret);