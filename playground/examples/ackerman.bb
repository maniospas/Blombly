final ack = {
    if(m > 0) {
        if(n == 0) return ack(m=m - 1; n=1);
        n=ack(m=m; n=n - 1);
        return ack(m=m - 1;n=n);
    } 
    return n + 1;
}


tic = time();
print(ack(m=3;n=7));
print(time()-tic);