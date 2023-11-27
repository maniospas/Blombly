context = new({
    print("See this message only once because we created a struct instead of a code block");
    final bias = 1;
    final inc = {
        return(add(x, bias));
    }
    return(self)
});
inc = new({context:return(inc)});
inc = new({context:return(inc)});
final bias = 2;
print(inc(x=1));