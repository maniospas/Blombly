n = 1000000;
a = Vector(n);
b = Vector(n);
c = Vector(n);

tic = time();
i = 0;
n = len(c);
while(i<n,
    //c[i]=a[i]+b[i];
    if(i<1, print("test"));
    i = i+1;
);
print(time()-tic);