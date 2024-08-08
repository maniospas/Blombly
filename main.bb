point = new {
  \x = 0;
  \y = 0;
  final set = {
    if(x as x) // effectively checks if 
      this\x = x;
    if(y as y)
      this\y = y;
  }
  final add = {return this\x + this\y}
}

point.set(x=1);
print(point.add());
print(point\x); // CREATES AN ERROR