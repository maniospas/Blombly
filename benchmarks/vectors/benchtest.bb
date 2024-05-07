n = 10000000;
a = Vector(n)+0.1;
b = Vector(n)+0.1;

tic = time();
c = a+b;
print(time()-tic);