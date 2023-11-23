x = 9;
y = 6;

test = {
    test2 = {
        z = add(x,y);
        z = add(1,z);
        return(z);
    }
    z = test2();
    return(z);
}

args = {x=add(x,1)}
k = test(args);
print(k);
print(x); 
