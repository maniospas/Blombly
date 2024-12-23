A = new {
    elements = vector(128);
    dosum() = {return sum(this.elements)}
    modify(int position) = {
        default value=0;
        this.elements[position]=value;
        return this;
    }
}

A = A.modify(1 :: value=2);
print(A.elements);
