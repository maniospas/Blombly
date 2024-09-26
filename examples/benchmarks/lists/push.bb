#include "libs/std"
enable std;


n = 10000000;
print("-------------------------------------------");
print("Benchmark: lists with "+str(n)+" elements");
print("Language: blombly");
print("-------------------------------------------");

x = list(1);
tic = time();
i = 0;
while(i<n){
    i = i+1;
    push(x, i);
}
print("  push "+str(time()-tic));

tic = time();
s = 0;
i = 0;
while(i<n) {
	s = s+x[i];
	i = i+1;
}
print("  access sum "+str(time()-tic));

tic = time();
it = iter(x);
s = 0;
while(xi as next(it)) 
	s = s+xi;
print("  iter sum "+str(time()-tic));


print("-------------------------------------------");
print("Benchmark: lists with "+str(n)+" elements");
print("Language: blombly + vector");
print("-------------------------------------------");

x = vector(x);

tic = time();
s = 0;
i = 0;
while(i<n) {
	s = s+x[i];
	i = i+1;
}
print("  access sum "+str(time()-tic));

tic = time();
it = iter(x);
s = 0;
while(xi as next(it)) 
	s = s+xi;
print("  iter sum "+str(time()-tic));
print("");