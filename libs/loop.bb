final loop::INFO as {
    name    = "loop";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.2";
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
    \n loop::lambda
    \n ------------
    \n Creates an anonymous function that transforms some
    \n arguments to the outcome of an *expression*. The
    \n arguments and expressions are internally separated
    \n with `->`. Here is an example in action:
    \n
    \n |   inc = loop::lambda(x->x+1);
    \n |   print(inc(0));  // 1
    \n
    \n loop::tolist
    \n ------------
    \n Gathers all the values of an iterable and converts
    \n them to a list. There are two variations to optionally
    \n apply element-wise transforms to the lists with the 
    \n semi-typing notation.
    \n Here is the pattern of gathering elements only:
    \n 
    \n |   iterable = std::range(5);
    \n |   A = loop::tolist(iterable);
    \n |   print(A);
    \n
    \n Next is an example of how to apply a transformation.
    \n This is consistent with typical transformation notation.
    \n 
    \n |   iterable = std::range(5);
    \n |   A = loop::tolist(iterable|str);
    \n |   print(A);
    \n 
    \n Finally, there is no need to explicitly use the 
    \n `loop::lambda` prefix to declare lambda for
    \n element-wise transformations. So you can do something
    \n like this:
    \n 
    \n |   iterable = 1, 2, 3;
    \n |   A = loop::tolist(iterable|x->x+1);
    \n |   print(A);
    \n
    ";
}

// iterators
#macro {loop::next(@name)} as {std::next(#of std::iter(@name))}
#macro {loop::range(@data)} as {std::next(#of std::range(@data))}

// lambdas
#macro {symb::lambda(@lhs->@rhs);} as {new {@transform(@lhs) = {return @rhs} return @transform}}

// list convertion
#macro {loop::tolist(@generator)} as {
    new {
        @list = @generator;
        @var = list();
        while(@element as loop::next(@list)) 
            push(@var, @element);
        return @var;
    }
}

// list convertion with semi-types
#macro {loop::tolist(@generator|@transform)} as {
    new {
        @list = @generator;
        @var = list();
        while(@element as loop::next(@list)) 
            if(@value as @element | @transform)
                push(@var, @value);
        return @var;
    }
}

// list convertion with lambda semi-types
#macro {loop::tolist(@generator|@lhs->@rhs)} as {
    new {
        @transform(@lhs) = {return @rhs}
        @list = @generator;
        @var = list();
        while(@element as loop::next(@list)) 
            if(@value as @element | @transform)
                push(@var, @value);
        return @var;
    }
}
