final Point2d = {
    final norm2d = {return this.x+this.y;}
    norm = norm2d;
}
final Point3d = {
    final norm3d = {return this.x+this.y+this.z;}
    norm = norm3d;
}

x = new {
    p = new {
        Point2d:
        Point3d:
        x=1;
        y=2;
        z=0;
    }
}
p = x.p.norm();
print("result "+str 1.2);