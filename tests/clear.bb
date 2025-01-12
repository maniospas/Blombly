// test clear
A = new{
    x = 1;
    y = 2;
}

assert A.x==1;
clear(A);
err = try print(A.x);
assert try catch(err) return true else return false;


// test move
B = new{
    x = 1;
    y = 2;
}
C = B|move;
err = try print(B.x);
assert try catch(err) return true else return false;
assert C.x==1;