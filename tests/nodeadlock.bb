// as long as we don't have infinite recursion Blombly should correctly handle complicated cases

final A = new {
    foo1() => extern2(B);
    foo2() => extern3(this);
    foo3() => B.foo1();
}

final B = new {
    foo1() => extern2(A);
    foo2() => extern3(this);
    foo3() => 1;
}

final extern1(obj) => obj.foo1();
final extern2(obj) => obj.foo2();
final extern3(obj) => obj.foo3();

assert extern1(A)==1;
