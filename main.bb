final Point = {
    norm2d = {
        default(exp=0.5);
        return (this.x^2+this.y^2)^exp;
    }
    is3d = false;
    norm = norm2d;
}

final Point3d = {
    norm3d = {
        default(exp=0.5);
        d = this.norm2d(exp=1);
        return (d+this.z^2)^exp;
    }
    Point:
    is3d = true;
    norm = norm3d;
}

p = new(Point3d:x=1;y=2;z=1);
if(p.is3d,
    print(p.norm());,
    print("2d");
);