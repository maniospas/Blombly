final fmt::INFO as {
    #spec name    = "fmt";
    #spec author  = "Emmanouil (Manios) Krasanakis";
    #spec license = "Apache 2.0";
    #spec version = "v1.0.0";
    #spec doc     = "
    This library introduces console messages that print 
    multiple arguments. New symbols:
    - `fmt::print(args)` concatenates a list of args into 
      a string that is printed.
    - `fmt::read(args)` concatenates a list of args into a 
      string that is printed before waiting for user input.
    - `fmt::fmt(args)` concatenates a list of args into a 
      string that is returned.
    - `fmt::ENABLE;` removes the `fmt::` prefix from the
      above macros. This overwrites the `print` and `read`
      symbols.";
}
// -----------------------------------------------------


final fmt::fmt = {
    formatted = "";
    argiter = std::iter(args);
    while(arg as std::next(argiter)) {
        formatted = formatted + std::str(arg);
        formatted = formatted + " ";  
    }
    return formatted;
}

#macro {fmt::print(@code)} as {std::print(fmt::fmt(@code))}
#macro {fmt::read(@code)} as {std::read(fmt::fmt(@code))}

final fmt::out = new {
    final fmt::fmt = fmt::fmt;
    \next = {fmt::read(arg)}
    push(arg) = {fmt::print(arg)}
}

#macro {fmt::ENABLE;} as {
    #macro {out} as {fmt::out}
    #macro {read} as {fmt::read}
    #macro {print} as {fmt::print}
    #macro {fmt} as {fmt::fmt}
}