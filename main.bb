n = 1000000;
a = Vector(n)+0.1;

tic = time();
//i = 0;
//n = len(c);
//while(i<n,
    //c[i]=a[i]+b[i];
    //if(i<1, print("test"));
    //i = i+1;
//);
it = iter(a);
s = 0;
while(i=next(it),
    s = s+i;
);
print(time()-tic);