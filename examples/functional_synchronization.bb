final DynamicPoint = {x=x;y=y;} // need to declare statics for calling from within blocks
final Increment = {
    inc = {
        default(value=1);
        z = self.x+value;
        self.x = z;
        return(self);  // need to return self to enable the synchronization pattern
    }
}
final Normed2D = {
    norm = {return(self.x^q+self.y^q);}  // need to use self. to "see" non-final variables
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
