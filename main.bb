#include "oop"

final test = new {
    final method = "A";
    fn test() {
        print(method);
    }
    return test;
}

final method = "B";
test();