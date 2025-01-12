# Code blocks

The main logic compartmentalization mechanism in Blombly are code blocks; these are flexible coding segments that can be treated as functions, 
used to define various control flows, or called inline. 
Code blocks are declared by enclosing some code in brackets and assigning them to a variable. 
There is no trailing semicolon, and the compiler will create an error if you do try to add one so that only one syntax is promoted. 
Block declaration only sets a variable and does not execute code.


## Inlining

"Paste" a block's internal code to the current position by using the block's name followed by double dots (`:`). 
This is called inlining and demonstrated below. The inlined block has full access to variables for usage and modification. 
This way, it enriches the current code with a snippet that is defined elsewhere, and which may also change dynamically.

```java
// main.bb
test = {
    print("Hello world"); 
} 
test:
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>


## Functions

Blombly can treat code blocks as functions by executing some code inside a paranthesis
and start a new scope from the values to run the block's code. 
The last semicolon may be ommited from blocks,
so this mostly looks like keyword arguments seperated by semicolons (`;`). 
We later show a modification that accepts positional arguments too.
Use a `return` statement to yield back a value. This stops block
execution immediately. An example of running 
a code block follows.

```java 
// main.bb
adder = {
    return x + y;
}
result = adder(x=1;y=2); 
print(result);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
3
</pre>

Being able to execute code as part of the arguments allows more
dynamic patterns than regular keyword arguments, namely the reuse
of arbitrary preparations before calling methods. For example,
declare configuration blocks that generate the arguments 
and inline them within the calling parenthesis.
Blombly comes alongside several [macros](../advanced/preprocessor.md), one of which is `@signature => @expression`. This is a shorthand for code blocks that only return an expression's outcome.
Below is an example that combines several concepts.


```java
// main.bb
final adder => x + y + bias;
final suber => x - y + bias;
final increase = {if(increment) bias=1 else bias=0}
final nobias = {bias=0}

increment = true;
a = adder(increase:x=1;y=2);
b = suber(nobias:x=1;y=2);
print(a);
print(b);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
4
-1
</pre>

!!! info
    Blombly is free to run functions in parallel. Synchronize them by passing the output of one into another,
    or by enclosing a multiple calls in a `try` block.

## Execution closure

Once arguments are transferred and the code block starts running, the latter can view only final
variables of the scope from which it is called. This means that moving blocks between scopes
makes them adapt to the finals of the new scope. This lets the language internally
schedule certain block calls to run in parallel threads for speedup. Below is an example.

```java
// main.bb
scope = {
    final value = "Declaration scope";
    value_printer = {print(value)}
    value_printer();
    return value_printer;
}

final value = "Running scope";
value_printer = scope();
value_printer();
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Declaration scope
Running scope
</pre>


!!! info 
    The property of obtaining values from a surrounding context is broadly called *closure*.
    Contrary to most interpreted languages, Blombly aims for efficient memory management
    and parallelization. It therefore uses the scope at which functions run (not where they are
    defined) as their closure.
    It further has access only to final variables to retain the ability of automatically
    running functions in parallel.



## Positional arguments

Blocks can be called using comma-separated positional arguments. 
This is a common programming pattern, 
though we stress that it is better to execute code to determine configurations. 
Positional arguments are stored in a list called `args`. For safery, 
this is considered a language keyword and the compiler does not allow assigning to it.
You can then access list elements or use `args|next` to sequentially pop arguments.

<br>

A shorthand of the last practice is to add positional comma-separated variable names inside a parenthesis next to the block. 
You can still assign the block to other variables or inline it
within other blocks given that they have enough remainder elements in `args`.

```java
// main.bb
adder(x, y) = {return x+y} // parenthesis front-pops the values, could also write `adder(x,y)=>x+y;`
test = adder;
result = test(1, 2);
print(result);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
3
</pre>


Blombly mixes positional arguments and code execution by separating the two with double doubledots 
(`::`) inside the call parenthesis. These also require whitespaces before and after.
Positional arguments are created first from the calling scope, so further argument generation can modify them.

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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
5
</pre>


