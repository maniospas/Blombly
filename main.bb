x = list();

while(i in range(100000))
    push(x, i);


tic = bbvm::time();

s = 0;
while(i in x) {
    s = s+i;
}

toc = bbvm::time();
print(s);
print(toc-tic);