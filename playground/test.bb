tic = time();


foo = {
    A = list();
    while(i in range(1000000)) A << i;
    return A|len;
}

results = list();
while(i in range(10)) results << 0;
while(i in range(10)) results[i] = foo();

print(time()-tic);
print(results|vector|sum);