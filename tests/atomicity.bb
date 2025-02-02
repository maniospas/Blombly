final accum = new {
    value = 0;
    add(x) = {this.value += x}
}

do while(i in range(10)) accum.add(i);
assert accum.value == 45;
