#include "std/dict"

point = new {
  \x = 0;
  \y = 0;
  set = {
    default x = this\x;
    default y = this\y;
    this\x = x;
    this\y = y;
    return this;
  }
  add = {return this\x + this\y}
}
print(point);
point = point.set(x=1);
print(point.add());