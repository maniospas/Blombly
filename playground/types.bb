// FLAT TYPES ARE SIMPLE TYPES
// Flat variables must always be declared
// by their type. You can assign lists to
// flat types, which pops from the front
// until all elements are consumed.
//
// FLAT TYPES ARE SHORT SERIAL DATA
// Their variable names behave like 
// comma-seperated elements, and you
// can set a flat type to function
// arguments to allow passing tuple
// values.
//
// FLAT TYPES ARE A ZERO COST ABSTRACTION
// For example, if `a.x` where 
// `a` is set as flat, 
// was some variable `a_x` instead.

!flat Point(x,y);
!flat Array2D(0,1);
!flat Field(Point start, 2D end);

adder(Point a, Point b) = {
    x = a.x+b.x;
    y = a.y+b.y;
    return x,y;
}

Point p1 = 1,2;
Point p2 = p1;
Point p3 = adder(p1, p2);
print(p3);

Field f = p1,adder(p1,p2);
print(f.start);
print(f.start.y);
print(f.end.1);