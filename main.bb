Point2D = {
    add(other) => new {
        Point2D = this...Point2D; // needed to assign to Point2D so that inlining can see it again
        Point2D:
        x = this..x + other.x;  // no need for super
        y = this..y + other.y;
    }
    str() => "({this.x}, {this.y})"; 
}

p1 = new {Point2D:x=1;y=2} 
p2 = new {Point2D:x=2;y=3}
Point2D = {fail("Point2D has been invalidated")}
print(p1+p2);