final adder = new{}.call(x) => new{}.call(y) => !closure.x+y+z;

test(z) = {
    final z = z;  // any functions running now can access this value
    return adder(1)(2);
}

print(test(1));
print(test(2));
