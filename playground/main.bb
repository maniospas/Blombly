Point = { 
    norm => (this.x^2+this.y^2)^0.5;
    str => "({this.x}, {this.y})";
} 
XYSetter = { 
    setx(value) = {this.x = value;return this} 
    sety(value) = {this.y = value;return this}
} 
point = new {Point:XYSetter:x=0;y=0} 
point = point.sety(4);
print(point);
print(point.norm());