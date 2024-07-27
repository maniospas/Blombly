final add(x,y) = {
    default bias = 0;
    return x+y+bias;
}
ret = add(1,2|bias=1);
print(ret);