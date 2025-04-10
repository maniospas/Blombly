// testing error


foo(x,y) = {
    x += "foo";
    x = y;
    return x + y;
}

value = foo(1,2);
print(value);
