// main.bb
!namespace dims {
    var x;
    var y;
}
!namespace main {
    var x;
    var y;
}


!include { // keep namespace activations in here only
    !with dims: // subsequent x and y are now dims::x and dims::y 
    Point = {
        norm() => (this.x^2+this.y^2)^0.5;
        str() => "(!{this.x}, !{this.y})";
    }
    p = new {Point: x=3;y=4}
}

// the scope is the same so we still access p
!with main:
p.x = 0;
print(p);
print(p.dims::x);