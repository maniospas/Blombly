!include "libs"

Point = {
    add(p) = {
        super = this;
        Point = !closure.Point;
        return new {
            Point:
            x = super.x + p.x;
            y = super.y + p.y;
        }
    }
}

p1 = new {Point: x = 1; y = 2}
p2 = new {Point: x = 3; y = 4}

p3 = p1 + p2;  // Calls the overloaded add method
print(p3.x, p3.y);