final accum = new {
    value = 0;
    add(x) = {this.value += x}
}

add_accum(i) = {accum.add(i)} // run on a separate thread each if possible
while(i in range(10)) add_accum(i);
defer assert accum.value == 45;
