# Structs

This section describes the process with which to make structs, that is, objects that hold certain variables that can be accessed and set with the dot notation. Similarly to scopes, structs can have final variables. There are three mechanisms to keep in mind: a) usage of the new keyword to create a scope for declaring structs (all scope assignments are transferred to the produced struct), b) the this keyword from which struct members can be accessed from called code blocks, and c) private variables that are not exposed externally. In addition to these concepts, we also demonstrate usage of code blocks as constructors to be inlined within struct creation. Boilerplate best practices that support object-oriented programming are provided as macros in the "std/oop" include covered in section 3.1. For the time being, though, we stick to base language features.

## New

You can create a data structure (aka object) with the `new {@code}` syntax. This creates a new scope that sees its parent's variables but keeps track of all new assignments. A struct holding those assignments as fields is returned, unless another return statement interrupts this (more on this in section 3.3), where struct fields can be accessed with the dot . operator afterwards. The example below demonstrates field access.
Running the  code yields the following output. An error is created because new only retains the assignments inside; the resulting struct is detached from the creator scope and cannot "see" the variable zbias.

```java
main.bb
zbias = 0; 
y = 2; 
point = new { 
    x = 1; 
    y = y; // get y from the parent scope because it is not set locally, then set it locally 
    z = x+y+zbias; // x and y from local scope, zbias from the parent scope 
} 
point.x = 4; 
print(point.x); 
print(point.y); 
print(point.z); 
print(point.zbias); // CREATES AN ERROR
```

```bash
> blombly main.bb
4
2
3
(<ERROR>) Missing value: zbias 
        → get _bb12 point zbias main.bbvm line 20
```

Blocks declared within new have access to a final variable called `this` that holds the struct. Access to this as a means of setting and writing fields 
is possible only after the struct's creation concludes. Furthermore, unless exposed through a different mechanism, 
the compiler prevents access to `.this` with the dot notation. Here is an example:

```java
// main.bb
point = new { 
    sum2d = {return this.x+this.y;} 
    sum3d = {return this.sum2d()+this.z;} 
    x = 1; 
    y = 2; 
    z = 3; 
} 
print(point.sum3d());
```

Normal variable visibility rules apply; to let a struct's code blocks call each other, either make them final or access them from this as above. For conciseness and portability (inlining in non-struct methods), the first option is preferred. Boilerplate that applies best practices for object-oriented programming is provided by std/oop import in the form of macros (basically syntax enrichments). Related material can be found in section 3.1.

## Inlined code blocks as constructors

In Blombly, inlining can be used to treat code blocks as part of constructors. This is a generalization of multi-inheritance that allows any number of blocks to work together during struct definitions. Inline the declaration of member functions as in the following example:

```java
// main.bb
final Point = { 
    norm = {return (this.x^2+this.y^2)^0.5;} 
} 
final XYSetter = { 
    setx(value) = {this.x = value;} 
    sety(value) = {this.y = value;}
} 
point = new { Point: XYSetter: x = 0; y = 0; } 
point.sety(4);
print(point.norm());
```

To prevent code smells and maintain the principle of "one solution", the compiler does not accept the notation `new @block` where `@block` is a code block. Instead, inline that block per `new {@block:}`. Similarly, final members cannot be set (e.g., with `final x = ...`). That is, any declaration that is not made final during a struct definition cannot be made final in the future. This imposes a clear distinction between mutable and immutable properties.

## Private variables

It is often important to declare local variables that may not be directly exposed outside their enclosing structs. This promotes code safety in the form of hidden states that cannot be altered externally. Private variables are denoted with the \ prefix at the beginning of their name (e.g., \test is private). Aside from their scope, they can be accessed only if their name (including the slash) follows the keyword this specifically. In the last case, they are retrieved from the namesake object. These restrictions are enforced by the compiler but not during interpretation (so .bbvm files can circumvent code safety). For example, the following code declares an object with private variables that cannot be directly accessed externally.

```java
// main.bb
point = new { 
    \x = 0; 
    \y = 0; 
    set = { 
        default x = this\x; 
        default y = this\y; 
        this\x = x; 
        this\y = y; 
        return this; // needed for synchronized code 
    } 
    add = {return this\x + this\y} 
}

point = point.set(x=1); // signify that we are making an update 
print(point.add());
```


## Returning from new

Return statements from within `new {...}` statements change the retrieved value to something other than a struct. For example, the following snippet is a valid (though not efficient in terms of asymptotic complexity) method for recursively computing a term of the Fibonacci sequence. BlomblyVM always schedules `new` for direct execution, that is, without spawning a separate thread. In general, though, prefer usage of this statement to create structs.

```java
final fib = {  
    if (n < 2) return 1;  
    return new {n = n - 1; fib:} + new {n = n - 2; fib:};
}

tic = time();
print("Result", fib(n = 21));
print("Time", time() - tic);
```

One particular wrinkle to iron out in our understanding is what it means to return a code block that is later used as a method. This is the same behavior as returning from code blocks called as methods with the result being another block. To maintain memory safety, all returned blocks are detached from their declaring scope. While being scopeless, they can only be inlined.

However, they can also get re-attached to a new scope by either setting them as struct fields (in which case they are attached to the struct) or creating new blocks that inline them. An example of this pattern is presented below:

```java
// main.bb
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

final message = "Declared in external scope";
rebased_method = {method:}
obj.method = method;

rebased_method();
obj.method();
method();  // CREATES AN ERROR
```

Running the code demonstrates the handling of memory context and how returned blocks lose their attachment to a memory context:

```bash
> blombly main.bb
Declared in an object
Declared in external scope
ERROR: Memoryless code block cannot be called.
   !!! Returned code blocks are not attached to any memory context.
   Consider setting it as a field in an object.
```

The question then persists: how to create callables that depend on their own internal state? This can be achieved with operation overloading that turns structs into callables, covered next.


## Operator overloading


Blombly supports operation overloading, which allows you to define custom behavior for standard operations such as addition, subtraction, and even method calls on your structs. This provides a powerful way to create user-defined types that can be manipulated as if they were built-in types, leading to more expressive code.

Overloading is achieved by defining methods with specific names inside structs that are used when the corresponding operation is performed. The most commonly overloaded operations are arithmetic operators (`+`, `-`, `*`, `/`) and the call operator `()` for making objects callable.

Let's start with a basic example where we overload the `+` operator for a struct that represents a 2D point.
In this example, we define a method add that performs the addition of two points and returns a new point. When the + operator is used, the add method is invoked automatically.

```java
final Point = new {
    fn \add(p) {
        return new {
            x = this.x + p.x;
            y = this.y + p.y;
        };
    }
};

p1 = new {Point: x = 1; y = 2};
p2 = new {Point: x = 3; y = 4};

p3 = p1 + p2;  // Calls the overloaded add method
print(p3.x, p3.y);  // Outputs 4, 6
```

One of the most useful applications of overloading is making structs callable, meaning they can be used as functions. This is done by overloading the call operator ().

Below is an example where we define a Multiplier struct that can be called like a function:

```java
// main.bb
final Multiplier = new {
    factor = 2;
    fn \call(x) {
        return x * this.factor;
    }
};

mul = new {Multiplier: factor = 5};

result = mul(10);  // Calls the overloaded call method
print(result);  // Outputs 50
```

Overloading becomes even more powerful when combined with code blocks. 
You can define overloaded operators that inline code blocks dynamically during execution.In the next example, we overload multiplication (`*`) for a point struct to scale it by a factor. 
This demonstrates how overloading can be used together with error handling to catch invalid operations and gracefully handle them.

```java
//main.bb
final Point = new {
    fn \mul(factor) {
        if (factor == 0) fail("Factor cannot be zero");
        return new {
            x = this.x * factor;
            y = this.y * factor;
        };
    }
};

p = new {Point: x = 2; y = 3};
scaled = try p * 0;  // This will fail
catch(scaled) 
    print("Error: " + str(scaled));  // Outputs: Error: Factor cannot be zero
```