# Basic syntax

This is the syntax of the `blombly` language. You can also create parsers
that accept languages with different but equivalent syntax that compile to 
the same virtual machine instruction set.

1. [Code blocks](#code-blocks)
2. [Final variables](#final-variables)
3. [Inline execution](#inline-execution)
4. [New](#new)

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
    return(add(x,y));
}
kwargs = {
    x=add(x,1);
    y=2;
}
value = sum(kwargs); 
print(value);
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
value = sum(x=add(x,1), y=2);
```

## Final variables

When you set a value to a variable you can set the assignment to be final.
This indicates that the value will never be allowed to change in this scope
in the future, which makes it *thread-safe* and can thus be visible by code
blocks defined within the same scope. Here's an example:

```java
bias = 0; // is not yet final
final bias = add(bias, 1); // final after the assignement
final inc = {
    return(add(x, bias));
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

```java
inc(final x=5, final bias=2)
```


## Inline execution

Sometimes, you may want to call a code block and let it
access and alter your scope's variables. 
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

## New

A final case of calling code blocks is by letting them
access all variables of the surrounding scope (like
inline) but register modifications in a separate
memory context. This is performed with the `new`
keyword and can be done like this:

```java
inc_x = {
    x = add(x, 1);
    return(x);
} 
x = 0;
result = new(inc_x);
print(x); // still 0
print(result); // 1
```

:warning: Similarly to inline execution, `new` runs sequentially in the same thread.

The newlly created memory context is stored as a data structure
in a local datavariable called `self`. This lets you use `new` to create data
structures like in the following code. You can obtain field values (including
code block fields that can be used as methods) from data
structures with the dot operator like `struct.field`, which is also demonstrated below.

```java
x = 0;
y = 2;
point = new(x=1;y=y;return(self)); // don't forget to return self, can ommit the brackets
print(x); // still 0
print(point.x); // 1
```

:bulb: Inline execution lets you use code blocks as constructors. With this syntax,
you can declare objects that use multiple constructors, like in the following example.

```java
StaticPoint = {final x=x;final y=y} // ensure that x, y are immutable
Normed2D = {
    norm = {
        xq = pow(x, q);
        yq = pow(y, q);
        return(add(xq, yq));
    }
}
extx = 1;
point = new(
    x=extx;
    y=2;
    StaticPoint:
    Normed2D:
    return(self);
); // created object will not store extx (only locally declared variables are kept)
print(point.norm(q=2));
```