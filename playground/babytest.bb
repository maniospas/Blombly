tic = time();
n = 100000000;
print(n);
print("1000000000");
sum = 0;
i = 0;
while(i<n) {
    sum += i;
    i += 1;
}
print(sum);
print(time()-tic);