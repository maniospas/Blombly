final normalize = {
    default q = 2;
    norm = sum(value^q)^(1/q);
    return value/norm;
}

final dot = {
    return sum(x*y);
}

final list2vector = {
    n = len(list);
    vector = Vector(n);
    i = 0; 
    while(i<n, vector[i]=list[i]; i=i+1);
    return vector;
}

final similarity = {
    x = normalize(config:value=x);
    y = normalize(config:value=y);
    return sim(config:x=x, y=y);
}

x = list2vector(list=List(1, 1));
y = list2vector(list=List(1, 1));
L2 = {q=2}
ret = similarity(x=x, y=y, config=L2, sim=dot);
print(ret);
