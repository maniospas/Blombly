final DynamicPoint = {
    this.x=x;
    this.y=y;
} // need to declare statics for calling from within blocks
final Increment = {
    inc = {
        default(value=1);
        this.x = this.x+value;
        return this;  // need to return itself to enable the synchronization pattern
    }
}
final Normed2D = {
    norm = {
        base = this.x^q+this.y^q;
        return base^(1/q);
    }  // need to use fhis. to "see" non-final variables
}
final Point = {
    new(
        DynamicPoint:
        Increment:
        Normed2D:
    )
}

point = Point(x=1,y=2);
print(point.norm(q=2)); // 5
point = point.inc(); // reasignment makes sure that the function is executed before point.x 
print(point.x); // 2 (without reasignment to point, this is arbitrary)
print(point.norm(q=2)); // 8
