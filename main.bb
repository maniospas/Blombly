tic = std::time();

final x = list();
i = 0;
while(i<100000) {
    push(x, i);
    i = i+1;
}

print(std::time()-tic);