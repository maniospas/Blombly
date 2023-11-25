final bias = 1;
final inc_result = {
    result = add(result, bias);
}
final add_inc = {
    result = add(x, y);
    inline(inc_result);
    return(result);
}
print(add_inc({x=4;y=5;})); 