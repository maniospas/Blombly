createInc = {
    final bias = 1;
    inc = {
        return x+bias;
    }
    return inc;
}
inc = createInc();
print(inc(x=1));  // 2