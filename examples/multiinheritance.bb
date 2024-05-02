final DynamicPoint = {
    this.x=x; // could also be x=x
    this.y=y;
}
final Normed2D = {
    norm = {
        default q = 2;
        return this.x^q+this.y^q;
    }  // need to use this. to "see" non-final variables (othewise it would get them from the calling scope)
}
final Point = {
    new(
        DynamicPoint:
        Normed2D:
    )
}


args = {x=1;y=2}
point = Point(args); // , is the same as ; for calling
print(point.norm(q=2));