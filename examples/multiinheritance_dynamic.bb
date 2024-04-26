final DynamicPoint = {x=x;y=y;} // need to declare statics for calling from within blocks
final Normed2D = {
    norm = {return(self.x^q+self.y^q);}  // need to use self. to "see" non-final variables
}
final Point = {
    new(
        DynamicPoint:
        Normed2D:
    )
}

point = Point(x=1,y=2); // , is the same as ; for calling
print(point.norm(q=2));