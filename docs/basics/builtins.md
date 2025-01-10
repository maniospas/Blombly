# Introduction

This section covers Blombly commands that are used in writing sequential code. It includes concepts 
that may already be familiar, such as comments, variable handling, builtin datatypes, and flow control. 
But rarer features are added to the mix, like immutable variables, pipes, and trying until return.

## Comments

Blombly only has line comments that start with `//`. However, it supports multi-line strings and these can be used for 
documention as in the snippet below. Do not worry about bloating the size of intermediate representation files, as the
compiler optimizes away unused code segments that cannot produce side-effects.

```java
// main.bb
"This is multiline documentation 
that describes the current file.";
print("Hello world!"); // This is a line comment.
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>


## Builtins

Assign values to variables with the equality operator (`=`), like below. If a variable with the same name already exists in the current scope, its value is overwritten. 
Otherwise, a new variable is created. Subsequent code in the same scope will retrieve that assigned value when using the variable. Scopes refer to isolated execution contexts 
in which subsequent code can overwrite variable values. Each program starts from one initial scope, but new ones are created when creating new objects or calling methods.

There are several builtin data types that are directly incorporated in the language.
Exhaustively, these are `int`, `float`, `bool`, `str`, `list`, `vector`, `map`, `iter`, `code`, `struct`, `file`, `server`.
Here we start with the first four, and split the rest to dedicated pages, like the one describing [iterables](iterables.md).

```java
// main.bb
i = 1;     // int 
f = 1.2;   // float 
b = false; // bool (or true)
s = "this is a string";
```

Some well-known operations on the above types are listed below. These are computed as one might have come to learn from other programming
languages. Only difference to usual practices is that the `not` operation has higher priority than assignment.

| Category                 | Operations                              | Description                                              |
|--------------------------|----------------------------------------|----------------------------------------------------------|
| Assignment               | `(expression)`                         | Compute the expression first. Also used in method calls later. |
| Assignment               | `y=x`, `y as x`                        | The return values of assignments are covered below. |
| Conversion               | `typename(x)`                          | Everything can be converted to `str`, numbers can be converted from `str`. |
| Elements                 | `a[i]`, `a[i]=x`                       | Element get and set for strings. |
| Arithmetics              | `+`, `-`, `*`, `/` <br> `^` <br> `%`   | Basic arithmetics (division is floating-point). <br> Exponentiation. <br> Modulo for integers. |
| String operations        | `+`                                    | Concatenation.                                                                      |
| Comparisons              | `<`, `>`, `<=`, `>=` <br>  `==`, `!=`  | Inequality comparisons. <br> Equality comparisons.                   |
| Boolean operations       | `and`, `or` <br> `not`                 | Logical operations for booleans.  <br> Negation of any boolean value it prepends.   |



In addition to these operations, 
Blombly supports string literals as syntactic sugar; expressions enclosed in brackets
within strings are replaced with their evaluation and converted to `str`. For safety,
expression terminating symbols (like further brackets, colons, or semicolons) 
are not allowed within literals. Here is an example that contains several operations.

```java
// main.bb
x = int("1");
y = float("0.5");
print("Sum is {x+y}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Sum is 1.500000
</pre>


As a second example, format numbers using providing a string specification in the element access notation. This
returns a string containing the provided specification

```java
//main.bb
x = 1.3456;
print("Here is a number: " + x[".3f"]);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Here is a number: 1.346
</pre>


## Semi-types

A core aspect of the blombly language is dynamic typecasting that reinterprets
data as different formats. This ensures that variables contain
or are transformed to appropriate data, therefore introducing correctness guarantees
for subsequent code. At its core, this is equivalent to calling methods of one argument. 
However, code writting is simplified thanks to fewer symbols and less nesting.

Type conversions can be read as variable type semantics, 
which we dub semi-types.
The main notation is a vertical slash per `@value|@func`,
which in a vacuum is equivalent to calling `@func(@value)`. 
This has lower priority than all other symbols because the goal is
to convert speficic values. Below is an example that declares your
own string formatting [function](blocks.md) and applies it
after converting a string to a float number.

```java
fmt(x) => x[".3f"];
print("1.2345"|float|fmt);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1.234
</pre>


Similarly to numeric operations, the expression `variable |= convertion;` reassigns to a variable. 
In this case, however, the leftwise convertions is performed first, enabling the pattern
`variable |= convertion1|convertion2|...;` 
This notation is intentionally similar to 
[double turnstile](https://en.wikipedia.org/wiki/Double_turnstile) and may be thought as 
variable modeling some property.
Below is an example.

```java
x = "1";
x |= int;
print(x);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
</pre>





## Final variables

Variable values are made immutable by prepending the `final` keyword to their assignment. This prevents subsequent code from overwriting values
and exposes them to code running outside the current scope. For the time being, however, consider final as a code safety feature.
Here is an example of the error shown when attempting to overwrite a final value.

```java
// main.bb
final x = 0; 
x = x+1; // CREATES AN ERROR
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
<span style="color: cyan;">> ./blombly</span> main.bb --strip
 (<span style="color: red;">ERROR</span>) Cannot overwrite final value: x
    <span style="color: lightblue;">→</span>  add x x _bb2                                      main.bbvm line 4
</pre>


## Control flow

Control flow alters which code segments are executed next. Blombly offers similar options to most programming languages in terms of conditional branching, loops, deffering
execution for later, method calling,
and error handling. The first three of those are described here, whereas method calling is described [seperately](blocks.md) because it offers more options than other languages. 
Error handling and early returns with `try` are described below.

Conditionals take the form `if(@condition){@accept}else{@reject}` where `@` denotes code segments (by the way, this is the macro declaration notation). 
The condition must yield a boolean and makes the respective branch execute, where the `else` branch is optional.
You may ommit brackets for single-command segments, but put semicolons only at the end of commands.

```java
// main.bb
x = 1;
if (x>0) print("positive")  // no semicolon (because the command would end)
else if(x<0) print("negative");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
positive
</pre>

Similarly, loops take the form `while (condition) {@code}` and keep executing the code while the condition is `true`. 
To avoid ambiguity, there are no other ways in the language's core to declare a loop, albeit the `in` keywords allows
you to iterate through lists and the like. Again, you may ommit brackets if only one command runs.
Here is an example:

```java
// main.bb
counter = 0;
while (counter<5) {
    print("Counter is: " + str(counter));
    counter = counter + 1;
}
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Counter is 0
Counter is 1
Counter is 2
Counter is 3
Counter is 4
</pre>

Finally, deferring is a command that declares code but makes it run later. *The deferred code always runs*, even
if errors occur in the interim. Normally, it is executed just before return statements, including returning from
`new` statements creating structs.
In advanced settings, you can remove cyclic struct references. Here is a usage of defer where we ignore the brackers
of the deferred code block.

```java
// main.bb
A = new { // this is how structs are created
    x=0;
    defer x=1; // executed just before the end of struct creation
    print(x);
}
print(A.x);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
0
1
</pre>

## Signals

Computations like converting invalid strings to numbers, or using the `next` operator of iterators,
return errors as values. These make execution fail only if used in computations.

One way of handling errors -aside from the catch statement that will be covered below- is
via the `as` keyword. This performs an assignment without breaking normal code writting 
on encountering an error value, but by returning a true/false value depending on whether
an error was found. For example, below is a simple one-liner that retries
reading from the console until a number if provided. It also
use function-based typecasting (where `@str|@func` is equivalent to `@func(@str)`) to chain
function calls.

```java
while(not number as "Give a number:"|read|float) {}
print(number);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Give a number: number
Give a number: 12
12
</pre>


The `@result = try{@code}` pattercommand intercepts errors that indicate
unsuccessful algorithms, as well as return statements that indicate successful conclusion of
computations. Returning is the same mechanism as the one we [next](blocks.md) use to
return values from functions. Omit brackets when only one command is tried, 
to combine the interception mechanism with other control flows:

```java
// main.bb
x = "Give a number: "|read|float;
sgn = try if(x>=0) return 1 else return -1;
print("Sign is {sgn}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Give a number: 42
Sign is 1
</pre>


A similar syntax breaks away from loops below. Contrary to errors, 
it is lightweight to intercept returns. 
In other cases, prepend `try` to loop bodies to let internal returns skip the rest of the body. 

```java
// main.bb
counter = 0;
try while (true) {
    counter = counter + 1;
    print("Counter is: " + str(counter));
    if(counter==5) return;  // keeps exiting the code until intercepted by try
}
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Counter is 1
Counter is 2
Counter is 3
Counter is 4
Counter is 5
</pre>

We typically want to differentiate between try results that hold errors and those that do not. 
This is achieved through the `catch` statement, which is equivalent to a conditional statement that checks whether the condition is an error. 
Usage is demonstrated below, where the the return signal is intercepted to stop the loop immediately. If no value is returned, 
the result obtains an error value.

```java
start = "start:"|read|int;
end = "end:  "|read|int;
i = 0;
result = try while (i <= end) {
    i = i + 3;
    if (i >= start) return i;
}
print("Finished searching.");
catch (result) fail("Found nothing: {result|str}"); // creates a custom error on-demand
print("The least multiple of 3 in range [{start}, {end}] is: {result}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
start: 4
end:   100
Finished searching. 
The least multiple of 3 in range [4, 100] is: 6 

> <span style="color: cyan;">./blombly</span> main.bb
start: 4
end:   1
Finished searching. 
(<span style="color: red;"> ERROR </span>) Found nothing: No error or return statement intercepted with `try`.
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            playground/main.bb line 9
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            playground/main.bb line 9
   <span style="color: lightblue;">→</span>  catch(result)fail("Found nothing: "+str(            playground/main.bb line 9
</pre>
