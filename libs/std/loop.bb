#macro(std::for(@var in @iterable)@code;) = {  // @code ; covers bracketed code too
    try {
        @temp = iter(@iterable);
        while(@var as next(@temp)) @code;
    }
}
