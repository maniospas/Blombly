Struct = {
    x = 0;
    xthis => this.x;
    xclosure => this..x;
}
x = 1;
struct = new{Struct:}
x = 2;

bbvm::assert struct.xthis() == 0;
bbvm::assert struct.xclosure() == 1;