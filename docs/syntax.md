# Basic syntax

This is the syntax of the `blombly` language. You can also create parsers
that accept languages with different but equivalent syntax that compile to 
the same virtual machine instruction set.

1. [Code blocks](#code-blocks)
2. [Final variables](#final-variables)
3. [Inline execution](#inline-execution)
4. [Conditions and loops](#conditions-and-loops)
5. [Data sturctures](#data-structures)

## Code blocks
 
The language's main structure are code blocks. These represent a series of computations
to be executed on-demand by enclosing them in brackets. Blocks can be used like methods, 
as shown bellow.

```java
block = { 
    // does not run on declaration (like a method)
    print("Hello world!");
}
block(); // prints
block(); // prints again
```

Unsafe variables, like the above (we will later see how to make them thread-safe by
declaring their values as final or passing them as arguments)
cannot be accessed anywhere in any other block. This lets blocks execute in parallel
threads. You can transfer values as keyword arguments like so:

```java
x = 0;
sum = {
    return(x+y);
}
kwargs = {
    x=x+1;
    y=2;
}
value = sum(kwargs); // runs kwargs before passing them
print(value); // 3
print(x); // still 0
```

The above pattern runs `kwargs` on a copy of the external variable scope
(the copy keeps track of only changes and is very lightweight) and in
the same thread.
Then, any newlly assigned variables in the copy scope are set into the
`sum` and the latter is computed in a new thread.
The top-level scope is left unaffected throughout this process.

You can also pass an already created data structure instead of code block 
as keyword arguments. Inside the parenthesis of a call,
can also use a comma (`,`) in place of a semicolon
(`;`), ommit the last semicolon of the argument block,
and ommit its enclosing brackets (`{}`). For example, the following is also a valid
code block call and looks a lot like other programming languages:

```java
value = sum(x=x+1, y=2);
```

Keyword arguments take priority over any local variable reading,
and can be overwritten within methods. You can call the `default`
method to perform a set of operations that extract default values;
only outcomes that do not exist in your scope are kept.
Variable assignment priority is demonstrated in the following snippet:

```java
final x = 0;
inc = {
    default(bias=0);
    return(x+bias);
}
print(inc(x=1, bias=5)); // 6 - kwarg has top priority
print(inc(x=1)); // 1 - default used for missing kwarg
print(inc()); // 0 - final value of x is used
```


## Final variables

When you set a value to a variable you can set the assignment to be final.
This indicates that the value will never be allowed to change in this scope
in the future, which makes it *thread-safe* and can thus be visible by code
blocks defined within the same scope. Here's an example:

```java
bias = 0; // is not yet final
final bias = bias+1; // final after the assignement
final inc = {
    return(x+bias);
}
print(inc(x=4)); // 5 - can ommit trailing ; and inferable brakets (is equivalent to print(inc({x=4;}));)
print(inc(x=4, bias=2)); // 6 (can overwrite external value)
```

To let code blocks declared in the same scope call each other, 
you must also make them final, similarly to the above example. 
To write recursive blocks (i.e., that call themselves) you also 
need to make them final.

:bulb: Passing final values to code blocks as arguments does not
automatically make them final there too. Make the keyword argument 
declarations final if you want to make sure that they are never
changed internally like so:

## Inline execution

Sometimes, you may want to call a code block and let it
access *and alter any of your scope's variables*s. 
This is called *inline execution* and can be invoked by
the block's name ended with with a colon (:). When you do this,
you can ommit the semicolon at the end of the command.
For example, in the following snippet `result = inc_x:` 
is shorthand for `result = inc_x:;` that performs
inline execution of `inc_x`.

```java
inc_x = {
    x = add(x, 1);
    return("success");
} 
x = 0;
result = inc_x:
print(x); // 1
print(result);
```

:bulb: You can use inline execution to enrich your local context.

:warning: Inlining runs sequentially in the same thread.

Control flow statements (conditions and loops) also perform inline execution.

## Conditions and loops

Conditions and loops can run as shown in the following snippet. Their arguments
are code blocks, which are executed inline once the conditions are performed. This
means that execution is sequential in the same thread.

```java
x = 1;
condition = {x>=0;}
nonnegative = {print("non-negative");}
negative = {print("negative");}
if(condition, nonnegative, negative);

i = 0;
condition = {i<10;}
do = {
    i = i+1;
    print(i);
}
while(condition, do);
```

To simplify syntax, you can also ommit the brackets of code blocks defined within
conditions like so:

```java
x = 1;
if(x>=0, print("non-negative"), print("negative"));

i = 0;
while(i<10,
    i = i+1;
    print(i);
)

```

## Data structures

A final case of calling code blocks is by letting them
access all variables of the surrounding scope (like
inline) but storing any newly assigned variables in a data structure
to be returned. You can obtain structure field values from data
structures with the dot operator like `struct.field`, 
which is also demonstrated below. This operator can also retrieve code
blocks to be used normally as methods. Here's an example of a data
structure:

```java
x = 0;
y = 2;
point = new(x=1;y=y); // can ommit the brackets
print(x); // still 0
print(point.x); // 1
```

:warning: Similarly to inline execution, `new` runs sequentially in the same thread.

:bulb: Inline execution lets you use code blocks as constructors. With this syntax,
you can declare objects that use multiple constructors, like in the following example.

```java
StaticPoint = {final x=x;final y=y} // ensure that x, y are immutable
Normed2D = {
    norm = {return(x^q+y^q);}
}
extx = 1;
point = new(
    x=extx;
    y=2;
    StaticPoint:
    Normed2D:
); // created object will not store extx (only locally declared variables are kept)
print(point.norm(q=2));
```

When calling a code block that is obrained from a data structure,
final variables of the structure are still visible alongside the
`self` reference to the structure. This reference can let you
get and set non-final members of `self` using the dot operator.

:bulb: Getting and setting on the same base data structure can be
a means of continuous communication between functions. The operations
themselves are atomic (e.g., safe to wait for specific values),
but there is no guarantee that current values will persist within the
same method and it's suggested that they are retrieved locally once
to operate on.