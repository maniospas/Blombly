final PairAdder = {
    // Use this code block to declare "add" within some other code.
    // You can do this either 
    add = { 
        // If blocks with specific names are declared within objects, they overload operators.
        // Otherwise call add(args=List(other)) given some local values x, y.
        other = pop(args);
        x = x + other.x;
        y = y + other.y;
        return new(x=x;y=y;class:); // inline constructor
    }
}
final PairStr = {
    str = {
        return "("+str(x)+","+str(y)+")";
    }
}

final Vector = {
    // We need to write Adder=Adder, etc because new(...) detaches the surrounding scope
    // after objects are created. Thus, here we declare all methods that will be extracted
    // from the surrounding scope when Vector: is inlined. We also make all of them final
    // to let methods declared within the same new(...) as Vector: to access them if required.
    // Accessing them is required, for example, when method add of the Adder tries to create
    // a new class instance.
    final PairAdder = PairAdder;
    final PairStr = PairStr;
    final Vector = Vector; // we also need to copy Vector to the local context to use it
    final class = Vector; // sets up what is the class constructor needed by the PairAdder 
    PairAdder:
    PairStr:
}

NewVector = {
    // Create a vector by calling this block.
    // requires the Adder, Pair, and Vector to be final so that they are visible 
    // when NewVector is used as a method.
    new(x=x;y=y;Vector:)
}

a = NewVector(x=1;y=2); // calls the constuctor code block - all block calls are parallelized
b = new(x=2;y=3;Vector:); // inlines the object construction (no parallelization, but does not need Adder, Pair, Vector to be final)
c = a+b;
print(c);
