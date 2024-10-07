test = {
    A = 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15;
    return iter(A);
}

show = {
    while(i as next(it)) 
        print(i);
}

final it = test();
show();
show();