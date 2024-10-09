tic = std::time();

x = list();
i = 0;
while(i<1000000) {
    push(x, i);
    i = i+1;
}

print(std::time()-tic);