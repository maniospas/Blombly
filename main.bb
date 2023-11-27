context = new(
    print("See this message only once because we created a new struct instead of a code block");
    final bias = 1;
    y = 0;
    final inc = {
        set(self, y, 1);
        return(add(x, bias));
    }
    return(self)
);
final bias = 2;
print(context.bias);
print(context.inc(x=1));
print(context.y);