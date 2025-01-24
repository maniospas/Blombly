final Num = {
    int => this.value;
    lt(other) => this.value < other.value;
    str => "num !{this.value}";
}
num(x) => new {Num:
    print(x);
    value = x;
    print(value);
}

A = 5,2,3,4,1;

print(num(5));
