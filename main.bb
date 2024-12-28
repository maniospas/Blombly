Point = {
    add(other) = {
        super = this;
        Point = !closure.Point;
        return new {
            Point:
            x = super.x + other.x;
            y = super.y + other.y;
        }
    }
    str => "({this.x}, {this.y})";
    norm => (this.x^2+this.y^2)^0.5;
}

p1 = new {Point:x=1;y=2}
p2 = new {Point:x=2;y=3}
print(p1.norm()); // 2.236
print(p1+p2); // (3, 5)