final zbias = 0;
y = 2;
point = new {
  x = 1;
  y = y;         // get the value from the parent scope and then set it locally
  z = x+y+zbias; // gets the locally set values of x and y, zbias from the parent scope
}
point.x = 4;
print(point.x);  // 4
print(point.y);  // 2
print(point.z);  // 3
print(point.zbias); // CREATES AN ERROR