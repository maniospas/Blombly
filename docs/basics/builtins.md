# The basics

This section covers Blombly commands that are used in writing sequential code. It includes concepts 
that may already be familiar, such as comments, variable handling, builtin datatypes, and flow control. 
But rarer features are added to the mix, like immutable variables, pipes, and trying until return.

## Comments

Blombly only has line comments that start with `//`. However, it supports multi-line strings and these can be used for 
documentation as in the snippet below. Do not worry about bloating the size of intermediate representation files, as the
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


## Scopes

Scopes, which refer to isolated execution contexts. Each program starts from one initial scope, and 
new ones are entered with `new{@code}` -this also creates [structs](structs.md)- or when calling functions.
Assign values to variables per `@var = @value;`, which also creates the variables if they do not exist already. 
Subsequent code can normally overwrite variable values. Make them 
immutable by prepending the `final` keyword to their last assignment. This prevents overwrites by subsequent code
and exposes the variables to functions spawned in the scope. For now, consider immutability as a code safety feature.
Here is what invalid overwrites look like.

```java
// main.bb
final x = 0; 
x = x+1; // CREATES AN ERROR
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb --strip
 (<span style="color: red;">ERROR</span>) Cannot overwrite final value: x
    <span style="color: lightblue;">→</span>  add x x _bb2                                      main.bbvm line 4
</pre>



## Builtins

There are several builtin data types that are directly incorporated in the language.
Exhaustively, these are `int`, `float`, `bool`, `str`, `list`, `vector`, `map`, `iter`, `code`, `struct`, `file`, `server`, `sqlite`.
Here we start with the first four, and split the rest to dedicated pages, like the one describing [iterables](iterables.md).
Some well-known operations are implemented, computed as one might have come to learn from other programming
languages. Only difference to usual practices the existence of `as` assignments (more details later), and that element access
is overloaded by some data types. For example, format a float to a string of three decimal digits per `x[".3f"]`.

<details>
  <summary>Operations</summary>
  <table>
    <thead>
      <tr>
        <th>Category</th>
        <th>Operation</th>
        <th>Description</th>
      </tr>
    </thead>
    <tbody>
      <tr>
        <td>Assignment</td>
        <td><code>(expression)</code></td>
        <td>Compute the expression first. Also used in method calls later.</td>
      </tr>
      <tr>
        <td>Assignment</td>
        <td><code>y=x</code>, <code>y as x</code></td>
        <td>The return values of assignments are covered below.</td>
      </tr>
      <tr>
        <td>Conversion</td>
        <td><code>typename(x)</code></td>
        <td>Everything can be converted to <code>str</code>, numbers can be converted from <code>str</code>.</td>
      </tr>
      <tr>
        <td>Elements</td>
        <td><code>a[i]</code>, <code>a[i]=x</code></td>
        <td>Element get and set, for example for lists and maps.</td>
      </tr>
      <tr>
        <td>Arithmetics</td>
        <td><code>+</code>, <code>-</code>, <code>*</code>, <code>/</code><br><code>^</code><br><code>%</code></td>
        <td>Basic arithmetics (division is floating-point).<br>Exponentiation.<br>Modulo for integers.</td>
      </tr>
      <tr>
        <td>Self-assignment</td>
        <td><code>op=</code></td>
        <td>Replace <code>op</code> with any arithmetic, string, or boolean operation.</td>
      </tr>
      <tr>
        <td>String operations</td>
        <td><code>+</code></td>
        <td>Concatenation.</td>
      </tr>
      <tr>
        <td>Comparisons</td>
        <td><code>&lt;</code>, <code>&gt;</code>, <code>&lt;=</code>, <code>&gt;=</code><br><code>==</code>, <code>!=</code></td>
        <td>Inequality comparisons.<br>Equality comparisons.</td>
      </tr>
      <tr>
        <td>Boolean operations</td>
        <td><code>and</code>, <code>or</code><br><code>not</code></td>
        <td>Logical operations for booleans.<br>Negation of any boolean value it prepends.</td>
      </tr>
    </tbody>
  </table>
</details>



Blombly also supports string literals as syntactic sugar; expressions enclosed in brackets
within strings are replaced with their evaluation and converted to `str`. For safety,
symbols that terminate expressions (brackets, colons, or semicolons) 
are not allowed within literals. Here is an example that contains a literal and some operations.

```java
// main.bb
x = int("1");
y = float("0.5");
x += 1;
print("Sum is {x+y}");
print("Is it positive? {x+y>0}")
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Sum is 2.500000
Is it positive? true
</pre>



## Semi-types

Blombly offers the notation `@value|@func` as the simpler equivalent of
calling a function of one argument `@func(@value)`
while avoiding excessive parentheses. 
This has lower priority than all other symbols because the goal is
to convert specific values. Below is an example that declares your
own string formatting function and applies it
after converting a string to a float number.

```java
fmt(x) => x[".3f"];
print("1.2345"|float|fmt);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1.234
</pre>


Similarly to numeric operations, the expression `variable |= func;` reassigns to a variable. 
In this case, however, the leftwise function is applied first, enabling the pattern
`variable |= func1|func2|...;` 
This notation is intentionally similar to 
[double turnstile](https://en.wikipedia.org/wiki/Double_turnstile) and may be thought of as 
a way to indicate that a variable being able to model
some property through a series of transformations. For example a statement `x|=float;` indicates
that `x` can be converted to a float and will be treated thusly from thereon.
We call the modelled property that is common across function outputs
a *semi-type* given that it is semantically guaranteed for subsequent code.

<br>

Semi-types are a weak typing system in that they allow
automatic conversion between built-ins.
But they also bring strong typing conventions by applying specific 
transformations to ensure that a desired state
is maintained for subsequent code. Do not let this thinking
restrict you of using the dash (`|`) notation wherever possible.


## Control flow

Control flow alters which code segments are executed next. Blombly offers similar options to most programming languages in terms of conditional branching, loops, deferring
execution for later, function calling,
and error handling. Functions are described [elsewhere](blocks.md) because they offer more options than what is common.

<br>

Conditionals take the form `if(@condition){@accept}else{@reject}` where `@` denotes code segments or expressions. By the way,
this notation is also used by [macros](../advanced/preprocessor.md). 
The condition must yield a boolean and makes the respective branch execute, where the `else` branch is optional.
You may omit brackets for single-command segments, but put semicolons only at the end of commands.

<br>

Similarly, loops take the form `while (condition) {@code}` and keep executing the code while the condition is `true`. 
To avoid ambiguity, there are no other ways in the language's core to declare a loop, albeit the `in` keywords allows
you to iterate through lists and the like. Again, you may omit brackets if only one command runs.
Here is an example with both control flow options.

```java
// main.bb
counter = 0;
while (counter<5) {
    if(counter%2==0) print("{counter} is even")  // no semicolon (signifies that the command continues to `else`)
    else print("{counter} is odd");
    counter = counter + 1;
}
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
0 is even 
1 is odd 
2 is even 
3 is odd 
4 is even 
</pre>

Finally, deferring declares code to run later. *The deferred code is always executed*, even
if errors occur in the interim. Normally, defer occurs just before return statements, including returning from
`new` scopes that create structs. It is also applied upon entering and exiting `try` blocks.
In advanced settings, you can clear resources, remove cyclic struct references,
or completely clear struct contents. Here is a usage example, where we ignore the brackets
of the deferred code block for simplicity.

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

!!! warning
    Defer cannot contain return statements, as it may already be executed in response to return statements.

## Errors & returns

Computations like converting invalid strings to numbers, or using the `next` operator of iterators,
return errors as values. These make execution fail if used in computations.
One way of handling errors is
the `as` keyword. This performs an assignment without breaking normal code writing 
upon encountering an error value, but returns a true/false value depending on whether
an error was found. Below is a simple one-liner that retries
reading from the console until a number is provided.

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


The `@result = try{@code}` pattern intercepts errors that indicate
unsuccessful algorithms, as well as return statements that indicate successful conclusion of
computations. When entered and exited, it also waits for parallel function calls evoked in the code to conclude first 
and applies all defer statements. Returning is the same mechanism that yields values from functions - though
we have not covered this yet. Omit brackets when only one command is tried.

<br>

For example, let the interception mechanism interrupt control flow like this `sgn = try if(x>=0) return 1 else return -1;`.
A similar syntax breaks away from loops below. Contrary to errors, 
returning is lightweight to intercept. 
You could also prepend `try` to loop bodies to let internal returns skip the rest of the body - 
this would emulate other languages' *continue* just as the syntax below emulates *break*. 
Blombly does not have extra keywords to enforce only one way of interrupting execution.

<br>

You may further want to differentiate between try results that hold errors and those that do not. 
In those cases, use `catch` to check the outcome of trying, 
which is effectively a special conditional statement that checks whether the condition is an error.
Missing values are not considered errors for the purposes of this statement. 
Usage is demonstrated below, where the return signal is intercepted to stop the loop immediately. 
If no value or error is intercepted, the result becomes a missing value error that can be caught.
Catching errors does not need to occur immediately either.

```java
// main.bb
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

<pre style="font-size: 80%; background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto; overflow-x: auto;">
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
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  catch(result)fail("Found nothing: "+str(            main.bb line 9
</pre>

!!! info
    The `as` statement should be preferred for error handling.
    Use `try` only to intercept returns or to keep the program running upon unforeseen errors.

!!! warning
    The `try` statement synchronizes all concurrency in scope both when entered and when exited.
    This may reduce parallelization speedups, so do not overuse it.