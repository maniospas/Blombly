n = 10000000;
a = vector(n)+0.1;
b = vector(n)+0.1;

tic = time();
c = vector(n);
i = 0;
while (i<n) { 
    temp = b[i]+c[i];
    i = i+1;
}
print(time()-tic);