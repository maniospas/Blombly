final accum = new {
    this\value = 0;
    add(x) = {this\value += x;print(this\value, x)}
    float() => this\value;
}

add_accum(i) = {accum.add(i)} // run on a separate thread each if possible

i = 0;
while(i<10) {
    add_accum(i);
    i += 1;
}

defer print(accum|float); // all methods are asynchronous, so