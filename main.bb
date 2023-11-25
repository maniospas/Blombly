sum = {return(add(x,y));}
inc = {x=add(x,1);}
incsum = {return(sum(inc));}
print(incsum({x=1;y=2}));