final point = struct({
        x=1;
        y=2;
        manh={return(x+y);}
    });
print(get(point, x));
set(point, x, 2);
set(point, x, 3);
set(point, x, 4);
print(get(point, x));
