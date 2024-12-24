final Adder = {
    final Adder = Adder;
    defer final getThis->this; // do this at the end
    float -> this.element+bbvm::float(this.parent);
    str -> this|bbvm::float|bbvm::str;
    call(newElement) -> new{
        assert len(args) == 0;
        Adder:
        parent=getThis();
        element=newElement;
    }
}

adder(x) -> new{
    Adder: 
    element=x; 
    parent=0;
}

add = {
    ret = adder(0);
    while(x as args|next) {
        x |= float;
        ret = ret(x);
    }
    return ret;
}



print(add(1,2,3,4));