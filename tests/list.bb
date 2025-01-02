A = 1,2,3,4;
assert A[1] == 2;
assert A|next == 1;
assert A|next == 2;
assert A|pop == 4;
assert A|pop == 3;
assert A|len == 0;
push(A, 5);
assert A|pop == 5;
