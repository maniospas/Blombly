bb.sci.vector(it) = {
    default n = len(it);
    it |= iter;
    values = vector::alloc(n);
    while(i in range(n)) values[i] = next(it);
    return vector(values);
}

x = bb.sci.vector(random(42));
print(x);
