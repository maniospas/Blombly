# Structs

This section describes the process with which to create structs. These are objects that hold a scope whose variables are treated as object fields.
Fields are accessed and set with the dot notation and can be made final, like normal variables.

<br>

When working with structs, `new` creates a scope for declaring them, `this` accesses struct members when calling its code blocks,
a fullstop (`.`) accesses fields or values from the creation closure, and private variables starting with `\` are not exposed externally. 
All assignments during initialization are transferred to the produced struct. The latter is afterwards detached from its creating scope.
In addition to these above, we demonstrate usage of code blocks as constructors to be inlined within struct creation. 

!!! tip 
    Code blocks only contain code. Use structs to transfer state.

## New

Create a data structure (aka object) with the `new {@code}` syntax. This creates a new scope that sees its parent's variables but keeps track of all new assignments. A struct holding those assignments as fields is returned.

<br>

Struct fields can be accessed with the dot (`.`) operator afterwards. The example below demonstrates field access.
It also creates an error to demonstrate that `new` only retains the assignments inside it. In particular,
the created struct is at the end detached from the scope and does not grant access to its variables.

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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
4
2
3
(<span style="color: red;">ERROR</span>) Missing value: zbias 
        <span style="color: lightblue;">→</span>  get _bb12 point zbias                              main.bbvm line 20
</pre>

In Blombly, inlining can be used to treat code blocks as part of constructors. This is a generalization of multi-inheritance that allows any number of blocks to work together during struct definitions. Inline the declaration of member functions as in the following example.
To prevent code smells, the compiler does not accept the notation `new @block` where `@block` is a code block variable. 
Inline code per `new {@block:}`. Also for security, any field that is not made final during a struct creation cannot be made final in the future. 
This imposes a clear distinction between mutable and immutable fields.  

```java
Point = { 
    final norm => (this.x^2+this.y^2)^0.5;
} 
XYSetter = { 
    // setters return `this` for synchronization
    final setx(value) = {this.x = value;return this} 
    final sety(value) = {this.y = value;return this}
} 
point = new {Point:XYSetter:x=0;y=0} 
point = point.sety(4);
print(point.norm());
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(0, 4) 
4.000000 
</pre>




## Return from new

Interrupting struct creation with a return statement changes the yielded value to something else. It is thus equivalent to isolating the scope.
For example, the following snippet is a valid (though not efficient in terms of asymptotic complexity) method for recursively computing 
a term of the Fibonacci sequence *without function calls*.

```java
final fib = {  
    if(n < 2) return 1;  
    return new{n=n-1; fib:} + new{n=n-2; fib:}
}

tic = time();
result = fib(n=21);
toc = time();
print("Result !{result}");
print("Elapsed !{toc-tic} sec");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Result 17711 
Elapsed 0.051626 sec 
</pre>



## Methods

Code blocks declared within struct creation have access to a variable called `this` that represents the struct itself.
Such blocks play the role of callable methods similarly to how other blocks can be called as functions. Use the same
calling convention as functions too for positional arguments and for bringing part of the scope into method
execution. Below is an example.

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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
6
</pre>

The relation between the struct and the method retrieved with the dot notation
is maintained only within the current scope. But the block may be attached to another struct to serve as its method, or even
be completely detached from any struct when returned from a function call. 

## Private variables

It is often important to declare local variables that may not be directly exposed outside their enclosing structs. 
This promotes code safety in the form of hidden states that cannot be altered externally. 
Private variables are denoted with the slash (`\`) prefix at the beginning of their name. For example, `\test` is private. 
Once struct creation is completed, accessing private variables is possible only if their name (including the slash) follows `this`. 
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

Blombly supports operation overloading to let structs emulate other data structures. Do so by defining methods with specific names inside structs that are used when the corresponding operation is performed. The most commonly overloaded operations are arithmetic operators (`+`, `-`, `*`, `/`) and the call operator `()` for making objects callable.

<br>

Let us start with a basic example where we overload the addition operator for a struct that represents a 2D point;
define an appropriately named method that performs the addition of two points and returns a new one. 
When the addition operator is used, the namesake method is invoked automatically.

```java
// main.bb
final Point = { // made final to be accessible everywhere in the scope
    add(other) => {
        x = this.x + other.x;
        y = this.y + other.y;
        return new {Point:x=x;y=y}
    }
}

p1 = new {Point: x = 1; y = 2}
p2 = new {Point: x = 3; y = 4}

p3 = p1 + p2;  // Calls the overloaded add method
print(p3.x, p3.y);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
[4, 6]
</pre>

Structs can also be made callable by overloading the corresponding operator (`call`).
Below is an example where we define a *Multiplier* code block that can create structs
to be used like functions. 
Notice the usage of `defer` as a means of inserting a default factor
at the end of struct creation only if a factor has not been defined by that time.

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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
50
</pre>

!!! info
    All struct methods other than overloaded calls are executed synchronously to minimize side effects.

## Creation closure

Functions -be they called code blocks or callable structs- can access the final variables of their
calling scope. However, it is sometimes useful to retain values from the struct's creation scope.

<br>

One option is to use the pattern `@value = @value;` during struct creation.
However, this requires a lot of additional code to maintain information
and could be shadowed by other struct fields. For this reason, Blombly offers an
automatic way to bring external values to the struct; accessing them
like fields while using more than one dots after `this`. When doing so, each additional dot
injects a pattern from obtaining a value from an enclosing scope. For example, `this..value` 
indicates that the variable `value` is maintained from the scope in which the struct
was created.
The same pattern may be used in more levels of closure. Here is an example similar to the previous 2D point.
Thanks to creation closure, we can use `=>` to retain a small level of visual nesting.

```java
Point2D = {
    add(other) => new {
        // Point2D need to exist against to be transferred
        Point2D = this...Point2D;
        Point2D:
        x = this..x + other.x;
        y = this..y + other.y;
    }
    str() => "(!{this.x}, !{this.y})"; 
}

p1 = new {Point2D:x=1;y=2} 
p2 = new {Point2D:x=2;y=3}
Point2D = {fail("Point2D has been invalidated")} // invalidated
print(p1+p2);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(3,5)
</pre>

!!! tip
    To compute the number of fullstops, count the total number of brackets (`{`) and immediate return shorthands (`=>`)
    that you intent to escape from. For example, in the last snippet, the expression`this...Point2D`
    has three dots to escape from the three levels of closure in which `new {`, `add(other) =>`, `Point2D = {` run.
