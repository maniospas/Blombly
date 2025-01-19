A = new{
    x=1;
    inc() = {
        this.x += 1;
        return this;
    }
}


while(i in range(2)) {
    A = A.inc();
    print(A.x);
}