foo(x,y) = {
    default bias = 0;
    return x+y+bias;
}

print(foo(1,2 :: bias=3));