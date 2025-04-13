!flat Point(x,y);

add(Point a, Point b) = {
    x = a.x+b.x;
    y = a.y+b.y;
    return x,y;
}

norm(Point a) = {
    return (a.x^2+a.y^2)^0.5;
}

Point a = 1.5,2;
Point b = a;
Point c = add(a, b);
assert a.y==2;
assert norm(c)==5;
