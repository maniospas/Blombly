final fmt::fmt = {
    formatted = "";
    argiter = std::iter(args);
    while(arg as std::next(argiter)) {
        formatted = formatted + std::str(arg);
        formatted = formatted + " ";
    }
    return formatted;
}

#macro (fmt::print(@code)) = {std::print(fmt::fmt(@code))}
#macro (fmt::read(@code)) = {std::read(fmt::fmt(@code))}

#macro (enable fmt;) = {
    #macro (read) = {fmt::read}
    #macro (print) = {fmt::print}
    #macro (fmt) = {fmt::fmt}
}