final Vector = {
    final Vector = Vector;
    final x = x;
    final y = y;
    final add = {
        this = poll(args);
        other = poll(args);
        return new(this.x=x+other.x;this.y=y+other.y;Vector:);
    }
    final str = {
        ret = "{"+str(this.x)+","+str(this.y)+"}";
        return ret;
    }
}

a = new(x=1;y=2;Vector:);
b = new(x=31;y=42;Vector:);
c = a+b;
print(a, "+", b, "=", c);