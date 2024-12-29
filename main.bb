A = map();

test = new {
    float => 3;
}

A["X"] = 1;
A[2] = 2;
A[test] = 3;
print(A["X"], A[2], A[test]);