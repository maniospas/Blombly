accum = new {
    value = 0;
    add(x) = {this.value += x; print("added {x}  sum {this.value}"); return this}
}

while(i in range(10)) accum = accum.add(i);
defer print(accum.value);