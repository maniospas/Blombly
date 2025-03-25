final method1() = {
    print("method1");
    this.method = method2;
    this.method();
}
final method2() = {
    print("method2");
    this.method = method3;
    this.method();
}
final method3() = {
    print("method3");
}

A = new {
    method = method1;
}
A.method();
