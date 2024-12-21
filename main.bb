adder(x, y) = {
    default bias = 0;
    assert len(args) == 0;
    x |= float;
    y |= float;
    return x + y + bias;
}

print(adder(1, 2 :: bias=1));