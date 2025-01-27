inc(x) = {
    default bias = 1;
    return x + bias;
}
assert inc(0) == 1;
assert inc(0 :: bias=2) == 2;