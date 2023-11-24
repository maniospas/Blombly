block = {
    x = add(x, 1);
    return(add(x,y));
}
sum = block({x=1;y=2});
print(sum);