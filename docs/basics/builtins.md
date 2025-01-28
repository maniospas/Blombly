# The basics

This section covers Blombly commands that are used in writing sequential code. It includes concepts 
that may already be familiar, such as comments, variable handling, builtin datatypes, and flow control. 
But rarer features are added to the mix, like immutable variables, semi-types, and deferred execution.

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

Scopes refer to isolated execution contexts. Each program starts from one initial scope, and 
new ones are entered when creating structs or calling functions.
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
Exhaustively, these are `int`, `float`, `bool`, `str`, `list`, `vector`, `map`, `iter`, `code`, `struct`, `file`, `server`, `sqlite`, `graphics`, 
`error`. Here we start with the first four, and split the rest to dedicated pages.
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
        <td>Arithmetic</td>
        <td><code>+</code>, <code>-</code>, <code>*</code>, <code>/</code><br><code>^</code><br><code>%</code></td>
        <td>Basic arithmetic (division is floating-point).<br>Exponentiation.<br>Modulo for integers.</td>
      </tr>
      <tr>
        <td>Self-assignment</td>
        <td><code>op=</code></td>
        <td>Replace <code>op</code> with any arithmetic, string, or boolean operation.</td>
      </tr>
      <tr>
        <td>Iterable operations</td>
        <td><code>+</code><br><code>a&lt;&lt;i</code><br><code>pop(a)</code><br><code>next(a)</code></td>
        <td>String or list concatenation.<br>Push value `i` to `a` (often to the end, returns either the iterable or a result).<br>Extract the last value.<br>Extract the first value.</td>
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



Blombly also supports string interpolation as syntactic sugar: expressions enclosed in the pattern
`!{...}` within string literals are replaced at compile time with their evaluation and converted to `str`. 
For safety, symbols that terminate expressions (brackets, colons, or semicolons) 
are not allowed within linterpolation. Here is an example that contains several operations.

```java
// main.bb
x = int("1");
y = float("0.5");
x += 1;
print("Sum is !{x+y}");
print("Is it positive? !{x+y>0}")
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
// main.bb
// `=>` is a shorthand for functions that directly return
fmt(x) => x[".3f"];
print("1.2345"|float|fmt);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1.234
</pre>


Similarly to numeric operations, the expression `variable |= func;` reassigns to a variable. 
In this case, however, the leftwise function is applied first, enabling the pattern
`variable |= func1|func2|...;`.
We call the modelled property that is common across function outputs
a *semi-type* given that it is semantically guaranteed for subsequent code.
It brings strong typing conventions by ensuring that a desired state
is maintained for subsequent code. 


!!! info
    This notation is intentionally similar to 
    [double turnstile](https://en.wikipedia.org/wiki/Double_turnstile) and may be thought of as 
    a way to indicate that a variable being able to model
    some property through a series of transformations. For example a statement `x|=float;` indicates
    that `x` can be converted to a float and will be treated thusly from thereon.

!!! tip
    For easily readable code, use the dash (`|`) notation for all possible function calls of one argument while keeping at
    least one pair of parentheses like so: `value = float("number"|read);`
    This lets the outcome semi-type and the starting variable appear side-by-side, with intermediate preparatory steps following.


## Control flow

Control flow alters which code segments are executed next. Blombly offers similar options to most programming languages in terms of conditional branching, loops, deferring execution for later, function calling,
and error handling. The first three is described in this and the next section,
but functions are described [elsewhere](blocks.md) because they offer more options than what is common.

<br>

Conditionals take the form `if(@condition){@accept}else{@reject}` where `@` denotes code segments or expressions
and is the nnotation also used by [macros](../advanced/preprocessor.md). 
The condition must yield a boolean and makes the respective branch execute, where the `else` branch is optional.
You may omit brackets for single-command segments, but put semicolons only at the end of commands.

<br>

Similarly, loops take the form `while (condition) {@code}` and keep executing the code while the condition is `true`. 
To avoid ambiguity, there are no other ways in the language's core to declare a loop, albeit the `in` macro is a shorthand
for looping through iterables, like lists. Again, omit brackets if only one command runs.
Here is an example with both control flow options.

```java
// main.bb
i = 0;
while (i<5) {
    // no semicolon before `else`
    if(i%2==0) print("!{i} is even")
    else print("!{i} is odd");
    i += 1;
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

!!! tip
    Emulate *switch*, *continue*, or *break* 
    by intercepting return signals. This is described [later](../advanced/try.md).

## Defer

Deferring sets code to run later. *The deferred code is always executed*, even
if errors occur in the interim. Normally, defer occurs just before return statements, including returning from
`new` scopes that create structs. It is also applied upon entering and exiting `do` blocks.
In advanced settings, you can clear resources, remove cyclic struct references,
or completely clear struct contents. Here is a usage example, where we ignore the brackets
of the deferred code block for simplicity.

```java
// main.bb
// create a struct creation scope
A = new {
    x=0;
    defer x=1;
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
