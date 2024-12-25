# Structs

This section describes the process with which to make structs, that is, objects that hold certain variables that can be accessed and set with the dot notation. Similarly to scopes, structs can have final variables. 
Prefer using structs only to transer state between code block executions. Do not store intermediate computational values in struct fields if you plan to use them from multiple methods.

<div style="background-color: rgb(159, 236, 199); color: black; text-align: center; padding: 20px 0; font-size: 24px; border: 1px solid black;">
    Code blocks only transfer code. Use structs to transfer state.
</div>

<br>
There are three notations for working with structs: a) `new` creates a scope for declaring structs (all scope assignments are transferred to the produced struct), b) `this` accesses struct members from called code blocks, and c) private variables are not exposed externally. In addition to these concepts, here we demonstrate usage of code blocks as constructors to be inlined within struct creation. 

## New

You can create a data structure (aka object) with the `new {@code}` syntax. This creates a new scope that sees its parent's variables but keeps track of all new assignments. A struct holding those assignments as fields is returned, unless another return statement is encountered first. 

Struct fields can be accessed with the dot (`.`) operator afterwards. The example below demonstrates field access.
It also creates an error because `new` only retains the assignments inside it. The created struct is detached from its creating scope and cannot "see" any of its variables or blocks (e.g., used as functions), such as the variable `zbias` in the example.

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
    sum2d => this.x+this.y;
    sum3d => this.sum2d()+this.z;
    x = 1; 
    y = 2; 
    z = 3; 
} 
print(point.sum3d());
```

Normal variable visibility rules apply too; to let a struct's code blocks call each other, either make them final or access them from `this`, like above. For conciseness and portability (inlining in non-struct methods), the first option is preferred. Boilerplate that applies best practices for object-oriented programming is provided by std/oop import in the form of macros (basically syntax enrichments). Related material can be found in section 3.1.

## Inlined constructors

In Blombly, inlining can be used to treat code blocks as part of constructors. This is a generalization of multi-inheritance that allows any number of blocks to work together during struct definitions. Inline the declaration of member functions as in the following example.

```java
// main.bb
Point = { 
    norm = {return (this.x^2+this.y^2)^0.5;} 
} 
XYSetter = { 
    setx(value) = {this.x = value} 
    sety(value) = {this.y = value}
} 
point = new {Point: XYSetter: x = 0; y = 0} 
point.sety(4);
print(point.norm());
```

To prevent code smells, the compiler does not accept the notation `new @block` where `@block` is a code block. If you must, inline that block per `new {@block:}`. Similarly, 
final struct fields cannot be set (e.g., with `final a.value = ...`). That is, any field that is not made final during a struct definition cannot be made final in the future. 
This imposes a clear distinction between conceptually mutable and immutable properties. 

Similarly to code blocks structs are completely detached from their declaring scope after creation and reattached to the running scope upon execution.
You may sometimes want to bring values -mainly code blocks- from that context locally with the pattern `@name = @name;`. Below is an example where a
code block is overwritten in the top-level scope but a reference to it still resides within the constructed object for future use.

```java
Point = {
    Point = Point;
    str => "({this.x},{this.y})";
    copy = {
        super = this;
        Point = super.Point; // Point: will need to know what Point is
        return new {
            Point:
            x=super.x;
            y=super.y;
        }
    }
}
point = new{Point:x=1;y=2}
Point = {fail("Should never be called");}
point = point.copy();
print(point);
```

```text
> blombly main.bb
(1,2)
```


## Private variables

It is often important to declare local variables that may not be directly exposed outside their enclosing structs. 
This promotes code safety in the form of hidden states that cannot be altered externally. Private variables are denoted with the \ prefix at the beginning of their name (e.g., `\test` is private). 
Aside from their scope, they can be accessed only if their name (including the slash) follows the keyword this specifically. In the last case, they are retrieved from the namesake object. 

These restrictions are enforced by the compiler but not during interpretation (so .bbvm files can circumvent code safety in your own code).
For example, the following snippet declares an object with private variables that cannot be directly accessed externally.

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
    dimadd = {return this\x + this\y} 
}

point = point.set(x=1); // signify that we are making an update 
print(point.dimadd());
```


## Returning from new

Return statements from within `new {...}` may change the retrieved value to something other than the struct being created.
For example, the following snippet is a valid (though not efficient in terms of asymptotic complexity) method for recursively computing 
a term of the Fibonacci sequence. Blombly always schedules `new` for direct execution, that is, without spawning a separate thread.

```java
final fib = {  
    if (n < 2) return 1;  
    return new {n = n - 1; fib:} + new {n = n - 2; fib:};
}

tic = time();
print("Result", fib(n = 21));
print("Time", time() - tic);
```


## Operator overloading

Blombly supports operation overloading, which allows you to define custom behavior for your structs for standard operations like addition, subtraction, and even method calls. 
Therefore, it provides a powerful way to create user-defined types that can be manipulated as if they were built-in types, leading to more expressive code.

Overloading is achieved by defining methods with specific names inside structs that are used when the corresponding operation is performed. The most commonly overloaded operations are arithmetic operators (`+`, `-`, `*`, `/`) and the call operator `()` for making objects callable.

Let's start with a basic example where we overload the addition operator for a struct that represents a 2D point.
In this example, we define a method add that performs the addition of two points and returns a new point. 
When the addition operator is used, the `add` method is invoked automatically.

```java
Point = {
    final Point = Point;
    add(p) => new {
        Point:
        x = this.x + p.x;
        y = this.y + p.y;
    }
}

p1 = new {Point: x = 1; y = 2}
p2 = new {Point: x = 3; y = 4}

p3 = p1 + p2;  // Calls the overloaded add method
print(p3.x, p3.y);  // Outputs 4, 6
```

One of the most useful applications of overloading is making structs callable, meaning they can be used as functions. This is done by overloading the call operator (`call`).
Below is an example where we define a Multiplier struct that can be called like a function. Notice the usage of `defer` as a means of inserting a default factor
at the end of *new* only if no factor is defined by that time.

```java
// main.bb
Multiplier = {
    defer default factor = 2;
    call(x) => x * this.factor;
}

mul = new {Multiplier: factor = 5}
result = mul(10);  // Calls the overloaded call method
print(result);
```

```text
> main.bb
50
```

Overloading becomes even more powerful when combined with code blocks. 
You can define overloaded operators that inline code blocks dynamically during execution.In the next example, we overload multiplication (`*`) for a point struct to scale it by a factor. 
This demonstrates how overloading can be used together with error handling to catch invalid operations and gracefully handle them.

```java
//main.bb
Point = {
    final Point = Point;
    mul(factor) => new {
        if (factor == 0) fail("Factor cannot be zero");
        x = this.x * factor;
        y = this.y * factor;
    }
}

p = new {Point: x = 2; y = 3}
scaled = try p * 0;  // This will fail
catch(scaled) print("Error: " + str(scaled));  // Outputs: Error: Factor cannot be zero
```