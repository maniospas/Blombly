// as long as we don't have infinite recursion Blombly should correctly handle complicated cases

final B = new {
    foo3() => 1;
}

final extern3(obj) => obj.foo3();

print(extern3(B));

