final thread::INFO as {
    name    = "sync";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces the `sync::next` macro that allows
    \n iteration over all elements of a list or other
    \n iterable, like so:
    \n
    \n    A = 1,2,3;
    \n    while(x as loop::next(A)) 
    \n       print(x);
    \n
    \n This is a wrapper around `std::next` that creates 
    \n an iterator for the supplied expression just
    \n before the loop's command.
    ";
}

#macro {loop::next(@name)} as {std::next(#of iter(@name))}
