buff = 0;
tic = bbvm::time();
while(i in range(100000)) {
    buff += i;
}
toc = bbvm::time();
print((buff));
print((toc-tic), "sec");