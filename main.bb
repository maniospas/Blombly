Point2d = {
    this.norm2d = {
        return this.x+this.y;
    }
    norm = norm2d;
}

Point3d = {
    norm3d = {
        return this.x+this.y+this.z;
    }
    norm = this.norm3d;
}

print("Hello world!");