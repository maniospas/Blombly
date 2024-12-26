buff = 0;
tic = time();
while(i in 10000000|range) 
    buff += i;
toc = time();

print((buff));
print((toc - tic), "sec");
