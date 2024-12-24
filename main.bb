tic = time();
A = list();
while(i in range(10000))
    push(A, i);
toc = time();
print(toc-tic);

tic = time();
B = list();
while(i as next(A))
    push(B, i);
toc = time();
print(toc-tic);
