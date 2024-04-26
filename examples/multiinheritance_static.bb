StaticPoint = {final x=x;final y=y;} // ensure that x, y are immutable
Normed2D = {
    norm = {return(x^q+y^q);}
}
extx = 1;
point = new(
    x=extx;
    y=2;
    StaticPoint:
    Normed2D:
); // created object will not store extx (only locally declared variables are kept)
print(point.norm(q=2));