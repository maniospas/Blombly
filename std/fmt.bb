#macro (printfm (@code);) = (print(fmt(@code)););

final fmt = {
    formatted = "";
    while(arg as next(args)) {
        formatted = formatted + str(arg);
        formatted = formatted + " ";
    }
    return formatted;
}