// test clear
A = new{
    x = 1;
    y = 2;
}

assert A.x==1;
clear(A);
assert try catch(A.x) return true else return false;


// test move
B = new{
    x = 1;
    y = 2;
}
C = B|move;
assert try catch(B.x) return true else return false;
assert C.x==1;