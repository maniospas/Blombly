final A = {
    defer {
        default size = 32;
        elements = vector(size);
    }
    dosum() = {
        return sum(this.elements);
    }
    put(int position, float value) = {
        this.elements[position] = value;
        return this;
    }
    at(int position) = {
        return this.elements[position];
    }
}

a = new {A:size=64}
a[1] = 2;
print(a[1]);
print(a.elements|len);
print(a.dosum());
