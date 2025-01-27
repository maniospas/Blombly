final counter = new {
    \value = 0; // private
    inc() = {this\value += 1}
    int() => this\value;
}

final safediv(x,y) = {
    if(y==0) fail("Cannot divide by zero");
    counter.inc();
    return x/y;
}

if(result as safediv(1,2)) print(result);
if(result as safediv(1,3)) print(result);
if(result as safediv(1,0)) print(result);

print("Number of safe divisions: !{counter|int}");