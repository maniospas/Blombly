# Code blocks

The main logic compartmentalization mechanism in Blombly are code blocks; these are flexible coding segments that can be treated as methods, 
used to define various control flows, or called inline. 
Code blocks are declared by enclosing some code in brackets and assigning them to a variable. 

There is no trailing semicolon, and the compiler will create an error if you do try to add one so that only one syntax is promoted. 
Block declaration only sets a variable and does not execute code.


## Inlining

One can "paste" a block's internal code to the current position by using the block's name followed by double dots (`:`). 
This is called inlining and demonstrated below. The inlined block has full access to variables for usage and modification. 
This way, it enriches the current code with a snippet that is defined elsewhere, and which may also change dynamically.

```java
// main.bb
test = {
    print("Hello world"); 
} 
test:
```


## Specifications

One can declare and retrieve specifications from code blocks. 
In the simplest case, Blombly offers the dot . notation, like the language's [structs](structs.md) to read and write specification data. 
However, specifications differ from struct fields in that they declare immutable properties, like documentation and source code versions.
In the example below specifications are set for a code block. The final keyword is mandatory to signify their immutable nature.
 For logic safety, an error will occur otherwise.

```java
// main.bb
hello = {
    print("Hello world!"); 
} 
final tehellost.version = "1.0.0"; // setting metadata is always final 
final hello.license = "CC0";

print("Running test version {hello.version} under {hello.license} license...");
hello: // the block's code directly runs here
```

A shorthand for declaring specifications consists of defining them at the beginning of respective blocks using the `!spec` directive of the [preprocessor](../advanced/preprocessor.md). 
This firective cannot use any of the block's variables, but has normal access to the block's declaring scope. The next example is identical to the previous one.

```java
// main.bb
hello = { 
    !spec version = "1.0.0";
    !spec license = "CC0";
    print("Hello world!"); 
}

print("Running test version {hello.version} under {hello.license} license...");
hello:
```


## Method calling

Blombly can call code blocks by executing some code inside a paranthesis. Variables
declared inside the parenthesis are transferred to the called block. 
The last semicolon may be ommited from code blocks,
so this mostly looks like keyword arguments seperated by semicolons (`;`). 
We later show a modification that accepts positional arguments too.
Use a `return` statement to yield back a call value. Here is an example:

```java 
// main.bb
adder = {
    return x + y;
}
result = adder(x=1;y=2); 
print(result);
```

```bash
> blombly main.bb
3
```

Being able to execute code as part of the arguments allows more
dynamic patterns than regular keyword arguments, namely the reuse
of arbitrary preparations before calling methods. For example, one
can declare configuration blocks that generate the arguments 
and inline them within the calling parenthesis. Here is an example:

```java
// main.bb
final adder = {return x + y + bias}
final suber = {return x - y + bias}
final increase = {if(increment) bias=1 else bias=0}
final nobias = {bias=0}

increment = true;
a = adder(increase:x=1;y=2);
b = suber(nobias:x=1;y=2);
print(a);
print(b);
```

```bash
> blombly main.bb
4
-1
```

Once arguments are transferred and the code block starts running, it can view only final
variables of the scope from which it is called. This means that moving blocks between scopes
makes them adapt to the finals of the new scope. This lets the language internally
schedule certain block calls to run in parallel threads for speedup.


## Positional arguments

Blocks can be called using comma-separated positional arguments. 
This is a common programming pattern, 
though we stress that it is better to execute code to determine configurations. 
Positional arguments are stored in a list called `args`. For safery, 
this is considered a language keyword and the compiler does not allow its usage.
You can then access list elements or use `next` to obtain their values like so:

```java
// main.bb
adder = { 
    x = next(args); 
    y = next(args); 
    return x + y; 
}
result = adder(1, 2); 
print(result);
```

A shorthand for the above notation is to add positional variable names inside a parenthesis next to the block, 
separated by commas. You can still assign the block to other variables, as demonstrated below, or inline them
within other definitions given that they have the correct remainder `args`.

```java
// main.bb
adder(x, y) = { // shorthand for front-popping these values with next 
    return x + y; 
} 
test = adder; // can still transfer the definition this way
result = test(1, 2);
print(result);
```

## Mixed arguments

Blombly allows mixing of positional arguments and code execution by separating the two with double doubledots 
(`::`) inside the call parenthesis.

```java
// main.bb
adder(x, y) = {
    default wx = 1; 
    default wy = 1; 
    return x*wx + y*wy;
}
result = adder(1,2 :: wx=1;wy=2); 
print(result);
```

Positional arguments are created first from the calling scope, so further argument generation can modify them.
