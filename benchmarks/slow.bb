n = 10000000;
a = Vector(n);
b = Vector(n);

tic = time();
c = Vector(n);
i = 0;
n = len(c);
while(i<n, c[i]=a[i]+b[i];i=i+1);
print(time()-tic);