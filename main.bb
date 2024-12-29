buff = 0;
tic = time();
while(i in range(10000000)) buff += i;
toc = time();

print((toc - tic), "sec");
print((buff));
