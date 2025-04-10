final ack(m,n) = {
    if(m > 0) {
        if(n == 0) return ack(m - 1, 1);
        return ack(m - 1, ack(m, n - 1));
    } 
    return n + 1;
}

final ack2(m, n) = {
    stack = list();
    stack << m;
    while(m as stack|pop) {
        if (m == 0) {
            if (stack|len == 0) return n + 1;
            n = n + 1;
        } 
        else if (n == 0) {
            stack << m - 1;
            n = 1;
        } 
        else {
            stack << m - 1;
            stack << m;
            n = n - 1;
        }
    }
    return n;
}



tic = time();
print(ack(3,7));
print(time()-tic);

tic = time();
print(ack2(3,7));
print(time()-tic);