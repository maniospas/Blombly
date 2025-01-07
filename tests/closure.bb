Struct = {
    x = 0;
    xthis => this.x;
    xclosure => this..x;
}
x = 1;
struct = new{Struct:}
x = 2;

assert struct.xthis() == 0;
assert struct.xclosure() == 1;
