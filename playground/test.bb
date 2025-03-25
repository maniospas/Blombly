tic = time();

print(tic);

s = 0;
while(i in range(1000000))
    s += i;

print(time()-tic);
print(tic["%Y-%m-%d %H:%M:%S"]);
