// point.bb
final Point2D = {
    Point2D = Point2D;
    str => "({this.x}, {this.y})";
    add(other) = {
        super = this;
        return new {
            Point2D:
            x = super.x + other.x;
            y = super.y + other.y;
        }
    }
    dot(other) => this.x*other.x + this.y*other.y;
}

ab_points = {
    a = new {Point2D:x=0;y=0}
    b = new {Point2D:x=0;y=0}
}

bbvm::test("dot product") {ab_points:return a.dot(b)}
bbvm::test("addition") {ab_points:return a+b}