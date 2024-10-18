final loop::INFO as {
    name    = "loop";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.1";
    release = 0;
    year    = 2024;
    doc     = "
    \n Provides macros that declare anonymous iterators for
    \n faster code writting.
    \n 
    \n loop::next
    \n ----------
    \n Allows iteration over all elements of a list or 
    \n other iterable, like so:
    \n
    \n |   A = 1,2,3;
    \n |   while(x as loop::next(A)) 
    \n |      print(x);
    \n
    \n This is a wrapper around `std::next` that creates 
    \n an iterator for the supplied expression just
    \n before the loop's command.
    \n 
    \n loop:range
    \n ----------
    \n Performs in-place construction of an iterator with
    \n `std::range` and the same arguments and transfers that
    \n to the loop. For example:
    \n 
    \n |   while(i as loop::range(1, 5))
    \n |       print(i);
    \n
    ";
}

#macro {loop::next(@name)} as {std::next(#of std::iter(@name))}
#macro {loop::range(@data)} as {std::next(#of std::range(@data))}
#macro {next} as {
    #fail "`next` has been invalidated.
    \n  !!! The loop library introduces `loop::next` as an alternative to `std::next`.
    \n      The prefix-less symbol is invalidated to force explicit selection between the two."
}
#macro {range} as {
    #fail "`range` has been invalidated.
    \n  !!! The loop library introduces `loop::range` as an alternative to `std::range`.
    \n      The prefix-less symbol is invalidated to force explicit selection between the two."
}