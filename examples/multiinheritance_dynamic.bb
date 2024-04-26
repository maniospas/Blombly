final DynamicPoint = {
    this.x=x; // could also be x=x
    this.y=y;
}
final Normed2D = {
    norm = {
        return (this.x^q+this.y^q;
    }  // need to use this. to "see" non-final variables (othewise it would get them from the calling scope)
}
final Point = {
    new(
        DynamicPoint:
        Normed2D:
    )
}

point = Point(x=1,y=2); // , is the same as ; for calling
print(point.norm(q=2));