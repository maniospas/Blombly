A = new {
    x=0;
    defer x=1;
    print(x);
}
print(A.x);