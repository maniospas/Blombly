buff = "";
tic = bbvm::time();
while(i in range(10000)) {
    buff = buff+" "+str(i);
}
toc = bbvm::time();
print(len(buff));
print((toc-tic), "sec");