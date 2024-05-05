final Zeros = {
    default step = 10000;
    final step = step;
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
        a = list;
        b = other.list;
        ret = List();
        adder = {c[i] = a[i]+b[i]};
        i = 0;
        while(i<len(a),
            adder(a=a, b=b, c=ret, i=i);
            i = i+1;
        );
        ret = new(
            final list=ret;
            final step=step;
            final n=n;
            PVector:
        );
        return ret; // synchronize computations
    }
}

a = new(n=1000000;Zeros:PVector:);
b = new(n=1000000;Zeros:PVector:);

a[100000] = 1.2;
print(a[100000]);
tic = time();
c = a+b;
print(c[100000]);
print(time()-tic);