x = List();
tic = time();
i = 0;
while(i<1000000,
    i = i+1;
    push(x, i);
);
print(time()-tic);

tic = time();
s = 0;
i = 0;
while(i<1000000,
    s = s+x[i]/10000;
    i = i+1;
);
print(time()-tic);