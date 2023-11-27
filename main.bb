context = {
    final bias = 1;
    inc = {
        return(add(x, bias));
    }
}
inc = new({context:return(inc)});
final bias = 2;
print(inc(x=1));