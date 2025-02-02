final surface = new{x=0}
final surface2 = new{y=0}
add(a, b) = {
    defer surface2.y=1;
    a |= float;
    b |= float;
    surface.x = b;
    return 0;
}

result = add(0, "foo");
assert surface.x == 0;
assert surface2.y == 1;
assert do catch(result) return true else return false;