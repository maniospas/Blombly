tic = time();
ret = 0;
while(i in range(1000000000)) ret += i;
print(time()-tic);
print(ret);

// running time with blombly v1.18.0:  88 sec
// direct equivalent in Python 3.12.7: 43 
// running time with 