final PairAdder = {
    // Use this code block to declare "add" within some other code.
    // You can do this either 
    add = { 
        // If blocks with specific names are declared within objects, they overload operators.
        // Otherwise call add(args=List(other)) given some local values x, y.
        this = next(args); // mostly for compatibility with usage outside objects
        other = next(args);
        x = this.x + other.x;
        y = this.y + other.y;
        return new(x=x;y=y;class:); // inline constructor since our data structure is lightweight
    }
    incx = {
        default value=1; // the whole computation runs, but only values not existing in the context are kept 
        this.x = this.x+value; // `this` is declared as final during new(...)
        return this; // return this to allow functional synchronization
    }
}
final PairStr = {
    str = {"("+str(this.x)+","+str(this.y)+")"} // ommit any of the return or last questionmark for last block commands
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
    return new(x=x;y=y;Vector:);
}

a = NewVector(x=1;y=2); // calls the constuctor code block - all block calls are parallelized
b = new(x=2;y=3;Vector:); // inlines the object construction (no parallelization, but does not need Adder, Pair, Vector to be final)
c = a+b;
c = c.incx();
print(c);
