
final back(element) = {return new {
    final element = element;
    call(A) = {
        bbvm::push(A, element);
        return A;
    }
}}

final apply(func) = {return new {
    final func = func;
    call(A) = {
        ret = list();
        while(x in A)
            push(ret, func(x));
        return ret;
    }
}}
