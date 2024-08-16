#macro (std::print(@code);) = (print(std::fmt(@code)););

final std::fmt = {
    formatted = "";
    while(arg as next(args)) {
        formatted = formatted + str(arg);
        formatted = formatted + " ";
    }
    return formatted;
}