final foo() => foo();
final A = new{}

add(x,y) = {
    A.x = foo();
    return x+y;
}

print(add(1,2));