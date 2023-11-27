final bias = 1;
final inc_result = {
    result = add(result, bias);
}
final addinc = {
    result = add(x, y);
    inc_result:
    return(result);
}
print(addinc(x=4;y=5)); 