# Basic syntax

This is the syntax of the `blombly` language. You can create different parsers
with equivalent syntax.

1. [Code blocks](#code-blocks)
2. [Final variables](#final-variables)
3. [Inline execution](#inline-execution)

## Code blocks
 
The language's main structure are code blocks. These represent a series of computations
to be executed on-demand by enclosing them in brackets. Blocks can be used like methods, 
as shown bellow.

```javascript
block = { 
    // does not run on declaration (like a method)
    print("Hello world!");
}
block(); // prints
block(); // prints again
```

Unsafe variables, like the above (we will later see how to make them thread-safe by
declaring their values as final)
cannot be accessed anywhere in any other block. This lets blocks execute in parallel
threads. You can transfer values as keyword arguments like so:

```javascript
x = 0;
sum = {
    return(add(x,y));
}
kwargs = {
    x=add(x+1);
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

## Final variables

When you set a value to a variable you can set the assignment to be final.
This indicates that the value will never be allowed change in this scope
in the future, which makes it *thread-safe* and can thus be visible by called
blocks. Here's an example of a final variable that is visible from a running block.

```javascript
bias = 0; // is not yet final
final bias = add(bias, 1); // final after the assignement
final inc = {
    return(add(x, bias));
}
print(inc(x=4)); // 5 - can ommit trailing ; and inferable brakets (is equivalent to print(inc({x=4;}));)
print(inc(x=4;bias=2)); // 6 (can overwrite external value)
```

To let code blocks call each other, you must make them final,
similarly to the above example. This lets
you write secure code by controlling which functionality or variables
are exposed to called blocks. To write recursive blocks (i.e., that
call themselves) you also need to make them final.

:bulb: Make keyword arguments final to make sure that the match between
their name and value is never broken.

## Inline execution

Sometimes, you may want to call a code block and let it
alter your scope's variables. You can do this by calling the inline
method on the block object:

```javascript
inc_x = {
    x = add(x, 1);
    return("success");
} 
x = 0;
result = inline(inc_x);
print(x); // 1
print(result);
```

:warning: Inlining runs sequentially in the same thread.

## Derivded execution

A final case of calling code blocks is by letting them
experience all variables of the surrounding scope (like
inline) but not modify the latter. This is called derived
execution of the blocks and can be done like this:

```javascript
inc_x = {
    x = add(x, 1);
    return(x);
} 
x = 0;
result = derived(inc_x);
print(x); // still 0
print(result); // 1
```

:warning: Derived execution runs sequentially in the same thread.