// main.bb
scope = {
    final value = "Declaration scope";
    value_printer = {print(value)}
    value_printer();
    return value_printer;
}

final value = "Running scope";
value_printer = scope();
value_printer();