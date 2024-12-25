Point = {
    Point = Point;
    str => "({this.x},{this.y})";
    copy = {
        super = this;
        Point = super.Point; // Point: will need to know what Point is
        return new {
            Point:
            x=super.x;
            y=super.y;
        }
    }
}
point = new{Point:x=1;y=2}
Point = {fail("Should never be called");}
point = point.copy();
print(point);
