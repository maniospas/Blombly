x = list();
tic = time();
i = 0;
while(i<10000000){
    i = i+1;
    push(x, i);
}
print(time()-tic);

tic = time();
it = iter(x);
s = 0;
while(xi as next(it)) 
	s = s+xi;
print(time()-tic);