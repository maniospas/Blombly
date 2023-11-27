Point = {final x=x;final y=y}
Normed2D = {
    norm = {
        xq = pow(get(self, x), q);
        yq = pow(get(self, y), q);
        return(add(xq, yq));
    }
}
extx = 1;
point = new({x=extx;y=2;Point:Normed2D:return(self)}); 
point2 = new({point:return(self)});
extx = 2;
print(extx);
print(get(point2, x));