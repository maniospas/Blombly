A = 1,2,3;
s = 0;
while(x in A) s += x;

it = iter(A);
while(x as bbvm::next(it)) s -= x;

assert s==0;