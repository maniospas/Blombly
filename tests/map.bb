A = map();
A["test"] = 1;
assert A["test"] == 1;
clear(A);

A = map(
    ("a", 1), 
    ("b", 2), 
    ("c", 3));
assert A["a"] == 1;
assert A["b"] == 2;
assert A["c"] == 3;