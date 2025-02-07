namespace dims {
    var x;
    var y;
}

namespace plot {
    var x;
    var y;
}

with dims: // enable the dims namespace 
Point = {
    norm() => (this.x^2+this.y^2)^0.5;
    str() => "(!{this.x}, !{this.y})";
}

p = new {Point:x=1;y=2}
print(p);