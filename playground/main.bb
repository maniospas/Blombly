tic = time();
ret = 0;
while(i in range(10000000))
    ret += i;

print(time() - tic);
print(ret);
