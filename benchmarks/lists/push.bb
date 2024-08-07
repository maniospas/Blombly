x = list();
tic = time();
i = 0;
while(i<1000000){
    i = i+1;
    push(x, i);
}
print(time()-tic);

tic = time();
s = 0;
i = 0;
while(i<1000000) {
	s = s+x[i];
	i = i+1;
}
print(time()-tic);

tic = time();
it = iter(x);
s = 0;
while(xi as next(it)) 
	s = s+xi;
print(time()-tic);