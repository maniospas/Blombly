waslength(move A) => A|len;

A = 1,2,3;
assert A|len==3;
assert waslength(A)==3;
assert A|len==0;