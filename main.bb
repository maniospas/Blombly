x = 9;
y = 6;

test = {
    test2 = {
        z = add(1,add(x,add(x,y)));
        z = add(x, add(y,z));
        return(z);
    }
    z = test2();
    return(z);
}

kwargs = {x=add(x,1)}
k = test(kwargs);
print(k);
print(x); 
