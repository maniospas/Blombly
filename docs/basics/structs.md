# Structs

This section describes the process with which to make structs, that is, objects that hold certain variables that can be accessed and set with the dot notation. Similarly to scopes, structs can have final variables. 
Prefer using structs only to transer state between code block executions. Do not store intermediate computational values in struct fields if you plan to use them from multiple methods.

<div style="background-color: rgb(159, 236, 199); color: black; text-align: center; padding: 20px 0; font-size: 24px; border: 1px solid black;">
    Code blocks only contain code. Use structs to transfer state.
</div>

<br>
There are four notations for working with structs: 

- `new` creates a scope for declaring structs. 
- `this` accesses struct members when calling its code blocks. 
- `.` accesses struct members, multiple of them retain values from its creating scope.
- Private variables start with `\` and are not exposed externally. 

All assignments during initialization are transferred to the produced struct. The latter is afterwards detached from its creating scope.
In addition to these above, we demonstrate usage of code blocks as constructors to be inlined within struct creation. 

## New

Create a data structure (aka object) with the `new {@code}` syntax. This creates a new scope that sees its parent's variables but keeps track of all new assignments. A struct holding those assignments as fields is returned.

Struct fields can be accessed with the dot (`.`) operator afterwards. The example below demonstrates field access.
It also creates an error because `new` only retains the assignments inside it. The created struct is detached from its creating scope and cannot "see" any of 
its variables or blocks (e.g., used as functions), such as the variable `zbias` in the example.

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

Blocks declared within new have access to a variable called `this` that holds a representation of the struct. 

Here is an example:

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


Interrupting struct creation with a return statement can change the created value to something else, effectively doing so in an isolated scope.
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
This promotes code safety in the form of hidden states that cannot be altered externally. 
Private variables are denoted with the slash (`\`) prefix at the beginning of their name. For example, `\test` is private. 
Once struct creation is completed, accessing private variables is possible only if ther name (including the slash) follows `this`. 

These restrictions are enforced by the compiler but not during interpretation (so .bbvm files can be altered to circumvent code safety).
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

## Operator overloading

Blombly supports operation overloading to loet structs emulate other data structures. Do so by defining methods with specific names inside structs that are used when the corresponding operation is performed. The most commonly overloaded operations are arithmetic operators (`+`, `-`, `*`, `/`) and the call operator `()` for making objects callable.

Let's start with a basic example where we overload the addition operator for a struct that represents a 2D point.
In this example, define an appropriately named method that performs the addition of two points and returns a new one. 
When the addition operator is used, the addition's method is invoked automatically. Since structs
hold data autonomously, we need to bring knowledge of the code block generating points into the ones being
created with `Point=Point`. More on this and simplifications later.

```java
// main.bb
Point = {
    Point = Point;
    add(p) = {
        super = this;
        Point = this.Point; // needed for Point=Point to run in `new`
        return new {
            Point:
            x = super.x + p.x;
            y = super.y + p.y;
        }
    }
}

p1 = new {Point: x = 1; y = 2}
p2 = new {Point: x = 3; y = 4}

p3 = p1 + p2;  // Calls the overloaded add method
print(p3.x, p3.y);
```

```text
> main.bb
[4, 6]
```

Structs can be made callable by overloading the corresponding operator (`call`).
Below is an example where we define a Multiplier struct that can be called like a function. 
Notice the usage of `defer` as a means of inserting a default factor
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

## Definition closure

Running code blocks -or callable structs- can access the final variables of their
calling blocks. However, we already saw that it may useful to retain values 
from the struct's creation scope.

We previously did this with the pattern `@value = @value;`. Howver, this may be clunky
and could be shadowed by other struct fields. For this reason, Blombly offers an
automatic way to bring external values to the struct; access them
as members while using more than one dots. When doing so, each additional dot
injects a pattern from obtaining a value from an enclosing scope.

For example, below are two equivalent snippets, 
where the second one uses the new notation to bring 
values inside structs without unecessary effort.

```java
// main.bb (complex - DISCOURAGED UNLESS NECESSARY)
value = 1;
A = new {
    value = value; // bring the value to the created struct - the struct is detached from the surrounding scope afterwards
    float => this.value; // basically `float = {return this.value}` to overload float conversion
}
value = 2; // ignored as `value=value` is called upon `new`
print(A|float);
```

```java
// main.bb (simplified equivalent)
value = 1;
A = new {
    float => this..value; // escape from float and then from A
}
value = 2;
print(A|float);
```

```text
> blombly main.bb
1
```

The same pattern may be used in more levels of nesting. Here is an example similar to the previous 2D point.
But, even in the more complicate scenario, we retain the same small level of nesting.

```java
Point2D = {
    add(other) => new {
        Point2D = this...Point2D; // needed to assign to Point2D so that inlining can see it again
        Point2D:
        x = this..x + other.x;  // no need for super
        y = this..y + other.y;
    }
    str() => "({this.x}, {this.y})"; 
}

p1 = new {Point2D:x=1;y=2} 
p2 = new {Point2D:x=2;y=3}
Point2D = {fail("Point2D has been invalidated")}
print(p1+p2);
```

```text
> blombly main.bb
(3,5)
```
