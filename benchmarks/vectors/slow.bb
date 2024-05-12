n = 1000000;
a = Vector(n);
b = Vector(n);
c = Vector(n);

tic = time();
i = 0;
while(i<n, 
    a[i] = b[i]+c[i];
    i = i+1;
);
print(time()-tic);