#macro(for(@var in @iterable)@code) = (
    @temp = iter(@iterable);
    while(@var as next(@temp)) @code
);
