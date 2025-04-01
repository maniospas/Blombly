A = new {
    numadd(x,y) => x+y;
    nummul(x,y) => x*y;
}

B = new {
    numadd(x,y) => x+y;
    nummul(x,y) = {fail("No mul")}
}

print(A.numadd(1,2));
print(B.numadd(1,2));
