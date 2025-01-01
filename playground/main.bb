x = 0;
struct = new {
   x = 1;
   xthis = {return this.x}
   xclosure() = {return this..x}
}
x = 2; // ignored
print(struct.x);
print(struct.xthis());
print(struct.xclosure());
