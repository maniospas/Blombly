final console::INFO as {
    name    = "console";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "v1.0";
    release = 1;
    year    = 2024;
    doc     = "
    Introduces console messages that print multiple 
    arguments. New symbols:
    - `console::print(args)` concatenates a list of args 
      into a string that is printed.
    - `console::read(args)` concatenates a list of args 
      into a string that is printed before waiting for 
      user input.
    - `console::fmt(args)` concatenates a list of args 
      into a string that is returned.
    - `console::enable;` removes the `console::` prefix 
      from the above macros. This overwrites the `print` 
      and `read` symbols.";
} 


final console::fmt = {
    formatted = "";
    argiter = bbv::iter(args);
    while(arg as bbv::next(argiter)) {
        formatted = formatted + bbv::str(arg);
        formatted = formatted + " ";  
    }
    return formatted;
}


#macro {console::print(@code)} as {bbv::print(console::fmt(@code))}
#macro {console::read(@code)} as {bbv::read(console::fmt(@code))}

final console::out = new {
    final console::fmt = console::fmt;
    \next = {console::read(arg)}
    \push(arg) = {console::print(arg)}
}

#macro {console:} as {
    #macro {out} as {console::out}
    #macro {read} as {console::read}
    #macro {print} as {console::print}
    #macro {fmt} as {console::fmt}
    #macro {#include} as {#fail "There can be no include statement after console::enable;"}
}