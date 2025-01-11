accum = new {
    value = 0;
    add(x) = {this.value += x; print("added {x}  sum {this.value}"); return this}
}

try while(i in range(10)) accum.add(i);
print(accum.value);