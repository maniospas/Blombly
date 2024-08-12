final method = new {
    final message = "Declared in a first scope";
    final printer = {
        print(message);
    }
    return printer;
}
final obj = new {
    final message = "Declared in an object";
}

print(obj);

final message = "Declared in external scope";
rebased_method = {method:}
obj.method = method;

rebased_method();
obj.method();
method();  // CREATES AN ERROR