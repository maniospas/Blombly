arr = list();
rand = random(time());
while(i in range(10000)) arr << next(rand)*2000-1000;

bruteForce(a) = {
    l = len(a);
    m = 0;
    while(i in range(l)) {
        sum = 0;
        while(j in range(i, l)) {
            sum += a[j];
            if(sum > m) m = sum;
        }
    }
    return m;
}

start = time();
ret = bruteForce(arr);
end = time();
print(end-start);
print(ret);