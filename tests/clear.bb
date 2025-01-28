// test clear
A = new{
    x = 1;
    y = 2;
}

assert A.x==1;
clear(A);
assert do catch(A.x) return true else return false;


// test move
B = new{
    x = 1;
    y = 2;
}
C = B|move;
assert do catch(B.x) return true else return false;
assert C.x==1;