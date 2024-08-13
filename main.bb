#include "std/oop"

fn badd(x, y) {
    default bias = 0;
    return x+y+bias;
}
fn test(x, y) {
    push(args,x);
    push(args,y);
    badd:
    return value;
}

print(test(1,2|bias=2));