final threads = 4;

final Zeros = {
    final step = int(n/threads);
    final list = List();
    final n = n;
    num = int(n/step);
    i = 0;
    while(i<num, 
        start = i*step;
        if(end = (i+1)*step;end>n, end=n);
        push(list, Vector(end-start));
        i = i+1;
    );
}


final PVector = {
    final PVector = PVector; // grant access of this struct to its children
    final put = {
        pos = poll(args);
        id = int(pos/step);
        list[id][pos - id*step] = poll(args);
    }
    final len = {
        return n;
    }
    final at = {
        pos = poll(args);
        id = int(pos/step);
        return list[id][pos - id*step];
    }
    final add = {
        other = poll(args);
        ret = List();
        adder = {c[i] = a[i]+b[i]}
        i = 0;
        while(i<len(list),
            adder(a=list, b=other.list, c=ret, i=i);
            i = i+1;
        );
        ret = new(final list=ret; final step=step; final n=n; PVector:);
        return ret; // synchronize computations
    }
}

n = 10000000;
pos = 5;
a = new(n=n;Zeros:PVector:);
b = new(n=n;Zeros:PVector:);

a[pos] = 1.2;
print(a[pos]);
tic = time();
c = a+b;
print(c[pos]);
print(time()-tic);