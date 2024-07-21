Point2d = {
    norm2d = {
        return this.x+this.y;
    }
    norm = norm2d;
}

Point3d = {
    norm3d = {
        default{w=0}
        return this.x+this.y+this.z+w;
    }
    norm = norm3d;
}

x = new {
    Point2d:
    Point3d:
    x = 1;
    y = 2;
    z = 3;
}

print(x.norm(w=1));