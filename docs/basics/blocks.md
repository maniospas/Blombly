# Code blocks

The main logic compartmentalization mechanism in Blombly are code blocks. These are source code segments that can be treated 
as functions or inlined within running code. 
Code blocks are enclosed in brackets and assigned to a variable. This only sets the variable and does not execute code yet.


## Inlining

"Paste" a block's code to the current position by following the variable holding it with double dots (`:`),
as demonstrated below. This is called *inlining* and grants full access to the scopes variables for usage and modification. 
This way, the inlined block enriches current code with a snippet that is defined elsewhere, and which may also change dynamically.

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

!!! info
    Use the `@value = do @blockname:` syntax to intercept return statements, if those are expected from inlined
    blocks. This is described [later](../advanced/try.md).


## Functions

Blombly can treat code blocks as functions by executing some code inside a parenthesis.
This starts a new scope from the values to run the block's code. 
The last semicolon may be omitted from blocks,
so this mostly looks like keyword arguments separated by semicolons (`;`). 
We later show a modification that accepts positional arguments too.
Use a `return` statement to yield back a value. This stops block
execution immediately. An example of running 
a code block follows.

<br>

Being able to execute code as part of the arguments allows more
dynamic patterns than regular keyword arguments, namely the reuse
of arbitrary preparations before calling methods. For example,
declare configuration blocks that generate the arguments 
and inline them within the calling parenthesis.

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

## Immediate return

Blombly comes alongside several [macros](../advanced/preprocessor.md), 
one of which is `@signature => @expression`. This is a shorthand for code blocks 
that only return an expression's outcome.
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
    Contrary to most interpreted languages, like Python and JavaScript, 
    Blombly aims for efficient memory management
    and parallelization. This is achieved by restricting access only 
    to the final variables from the scope at which functions run (*not* where they are
    defined) as closure.



## Positional arguments

Blocks can be called using comma-separated positional arguments. 
This is a common programming pattern, 
though we stress that it is better to execute code to determine configurations. 
Positional arguments are stored in a list called `args`. For safety, 
this is considered a language keyword and the compiler does not allow assigning to it.
You can then access list elements or use `args|next` to sequentially pop arguments.

<br>

A shorthand of the last practice is to add positional comma-separated variable names inside a parenthesis next to the block. 
This front-pops the values. You can still assign the block to other variables or inline it
within other blocks given that they have enough remainder elements in `args`.

```java
// main.bb
// could also write `adder(x,y)=>x+y;`
adder(x, y) = {return x+y} 
test = adder;
result = test(1, 2);
print(result);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
3
</pre>


Blombly mixes positional arguments and code execution by separating the two with double colons 
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

