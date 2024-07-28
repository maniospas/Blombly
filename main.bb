final Point2d = {
    final norm2d = {return (this.x^2+this.y^2)^0.5;}
    norm = norm2d;
}
final Point3d = {
    final norm3d = {return (norm2d()^2+this.z^2)^0.5;}
    norm = norm3d;
}

p = new {
    Point2d:
    Point3d:
    x = 1;
    y = 2;
    z = 3;
}

print(p.norm());