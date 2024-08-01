final scaler = {
    default scale = 1;
    if(scale<=0)
       error("Scale should be greater than zero, but another value was given: "+str(scale));
}

final add(x,y) = {
    signal = try scaler:
    catch(signal)
        return 0;
    return (x + y)*scale;
}

result = add(1, 2 | scale=0-10);
print(result);
