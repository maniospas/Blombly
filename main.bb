StaticPoint = {final x=x;final y=y} 
Normed2D = {
    norm = {
        default(q=2);
        xq = pow(x, q);
        yq = pow(y, q); // testing
        return(add(xq, yq));
    }
}
extx = 1;
point = new(
    x=extx;
    y=2;
    StaticPoint:
    Normed2D:
    return(self);
); 
print(point.norm());