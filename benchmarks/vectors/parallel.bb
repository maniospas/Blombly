final Zeros = {
    default step = int(n/8);
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
        this = next(args);
        pos = next(args);
        id = int(pos/step);
        list[id][pos - id*step] = next(args);
    }
    final len = {
        return n;
    }
    final at = {
        this = next(args);
        pos = next(args);
        id = int(pos/step);
        return list[id][pos - id*step];
    }
    final add = {
        this = next(args);
        other = next(args);
        ret = List();
        adder = {c[i] = a[i]+b[i]}
        i = 0;
        while(i<len(list),
            adder(a=list, b=other.list, c=ret, i=i);
            i = i+1;
        );
        ret = new(
            list=ret;  // TODO: this is necessary right now to avoid an error
            final list=list;
            final step=step;
            final n=n;
            PVector:
        );
        return ret;
    }
}

n = 100000000;
pos = 5;
a = new(n=n;Zeros:PVector:);
b = new(n=n;Zeros:PVector:);

a[pos] = 1.2;
tic = time();
c = a+b;
result = c[pos];
print(time()-tic);