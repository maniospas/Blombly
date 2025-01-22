foo(x,y) = {
    default bias = 0;
    return x+y+bias;
}

assert foo(1,2 :: bias=3) == 6;