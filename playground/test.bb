A = vector::zero(1000);
A = vector::random(1000);
A = vector::alloc(1000);
A = vector::from();


!namespace vector {
    // builtin implementations
    var zero;
    var random;
    var alloc;
}

!namespace list {
    // builtin implementations
    var empty;
    var element;
}

!namespace map {
    // builtin implentation
    var empty;
    var pairs;
}

A = list::empty();
A = list::element(1); // list of single element
A = list::from();