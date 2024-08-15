#include "std/oop"

module xy {
    scale = {
        default factor = 1;
        if(factor<=0)  // Omit implied brackets
            fail("Scale factor should be > 0, got: "+str(factor));
        x = x*factor;
        y = y*factor;
    }
    normalize = {
        norm = (x^2+y^2)^0.5;
        if(norm==0)
            fail("Cannot normalize a zero norm");
        x = x/norm;
        y = y/norm;
    }
}


fn adder(x, y) {
    err = try transform: // `try` intercepts errors or returns, `:` inlines execution
    catch(err) {
        print("Intercepted an error: " +str(err));
        return 0;
    }
    return x+y;
}

result = adder(1,2 | transform=xy.scale; factor=1);  // Calls run in parallel
print(result);