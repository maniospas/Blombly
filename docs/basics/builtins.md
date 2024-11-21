# Introduction

This section covers Blombly commands that are used in writing sequential code. It includes familiar concepts 
like comments, variable handling, builtin datatypes, and flow control. 
But more features are added to the mix, like immutable variables, trying until return, and handling missing values.

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

## Builtins

Assign values to variables with the equality operator (`=`), like below. If a variable with the same name already exists in the current scope, its value is overwritten. 
Otherwise, a new variable is created. Subsequent code in the same scope will retrieve that assigned value when using the variable. Scopes refer to isolated execution contexts 
in which subsequent code can overwrite variable values. Each program starts from one initial scope, but new ones are created when creating new objects or calling methods.

There are several builtin data types that are directly incorporated in the language.
Exhaustively, these are `int`, `float`, `bool`, `str`, `list`, `vector`, `map`, `iter`, `code`, `struct`, `server`.
Here we start with the first four, and split the rest to dedicated pages, like the one describing [iterables](iterables.md).

```java
// main.bb
i = 1;     // int 
f = 1.2;   // float 
b = false; // bool (or true)
s = "this is a string literal";
```

Some well-known operations on the above types are listed below. These are computed as one might have come to learn from other programming
languages. Only difference to usual practices is that the `not` operation has higher priority than assignment.

| **Category**             | **Operations**                         | **Description**                                          |
|--------------------------|----------------------------------------|----------------------------------------------------------|
| **Assignment**           | `(expression)`                         | Compute the expression first. Also used in method calls later. |
| **Assignment**           | `y=x`, `y as x`                        | The `as` assignment is covered below. |
| **Conversion**           | `typename(x)`                          | Everything can be converted to `str`, numbers can be converted from `str`.  |
| **Elements**             | `a[i]`, `a[i]=x`                       | Element get and set for strings. |
| **Arithmetics**          | `+`, `-`, `*`, `/` <br> `^` <br> `%`   | Basic arithmetics (division is floating-point). <br> Exponentiation. <br> Modulo for integers. |
| **String operations**    | `+`                                    | Concatenation.                                                                      |
| **Comparisons**          | `<`, `>`, `<=`, `>=` <br>  `==`, `!=`  | Inequality comparisons. <br> Equality comparisons.                   |
| **Boolean operations**   | `and`, `or` <br> `not`                 | Logical operations for booleans.  <br> Negation of any boolean value it prepends.   |


Here is an example that contains some of these operations:

```java
// main.bb
x = int("1");
y = float("0.5");
print("Sum is " + str(x+y)); // there is no implicit typecasting
```

```bash
> Blombly main.bb
Sum is 1.500000
```

## String formatting

Blombly supports string literals in which expressions enclosed in brackets
are replaced with their evaluation and converted to `str`. For safety,
expression terminating symbols (like brackets, inlining colons, or semicolons) 
are not allowed within literals during parsing. However, you may still have 
simple control flow as long as you avoid such symbols, like 
for example `try if(x<0) return -1 else return 1` that is discussed later in this section.
With string literals, the previous paragraph's example may be rewritten as:

```java
// main.bb
x = int("1");
y = float("0.5");
print("Sum is {x+y}");
```

Format numbers using the element access notation by providing a string specification. Formatted numbers
returns a string and looks like this:

```java
//main.bb
x = 1.3456;
print("Here is a number: " + x[".3f"]);
```

```bash
> Blombly main.bb
Here is a number: 1.346
```

Formatting numbers within literals is discussed alongside advanced typecasting [here](../advanced/typecasting.md).
For now, we provide a first taste that the recommended syntax looks like below. Very briefly, a callable formatter is defined
and then applied within the literal with typecast notation `x | fmt`, which is equivalent to `fmt(x)`.

```java
fmt(x) = {return x[".3f"]}
x = 1;
print("This is a number {x | fmt}");  
```


## Final variables

Variable values are made immutable by prepending the `final` keyword to their assignment. This prevents subsequent code from overwriting values
and exposes them to code running outside the current scope. For the time being, however, consider final as a code safety feature.
Here is an example of the error shown when attempting to overwrite a final value.

```java
// main.bb
final x = 0; 
x = x+1; // CREATES AN ERROR
```

```bash
> Blombly main.bb
 (ERROR) Cannot overwrite final value: x
    → add x x _bb2 main.bbvm line 4
```

## Control flow

Control flow alters which code segments are executed next. Blombly offers similar options to most programming languages in terms of conditional branching, loops, method calling,
and error handling. The first two of those are described here, whereas method calling is described [seperately](blocks.md) because it offers more options than other languages. 
Error handling also has its own dedicated page [here](errors.md), though below give a first taste of the `try` keyword's dynamic usage in other cases.

Conditionals take the form `if(@condition){@accept}else{@reject}` where `@` denotes code segments (by the way, this is the macro declaration notation). 
The condition must yield a boolean and makes the respective branch execute, where the `else` branch is optional.
You may ommit brackets for single-command segments, but put semicolons only at the end of commands.

```java
// main.bb
x = 1;
if (x>0) 
    print("positive")  // no semicolon (because the command would end)
else if(x<0)
    print("negative");
```

Similarly, loops take the form `while (condition) {@code}` and keep executing the code while the condition is `true`. 
To avoid ambiguity, there are no other ways in the language's core to declare a loop. 
Again, you may ommit brackets if only one command runs.
Here is an example:

```java
// main.bb
counter = 0;
while (counter<10) {
    print("Counter is: " + str(counter));
    counter = counter + 1;
}
```

## Try until return

Here we make a soft introduction to return signals; if errors indicate
unsuccessful algorithms, return statements indicate successful conclusion of
computations. The introduction focuses on returning (not on error handling), as
this supports what other languages dub as continue and break statements 
without needing such keywords.

To intercept return or error signals, use the `value = try{@code}` pattern. 
This is the same mechanism as the one we [next](blocks.md) use to
return values from called methods and unwind error stack traces.
By ommiting brackets when only one command is tried, we can conveniently
combine the interception mechanism with other control flows like so:

```java
// main.bb
x = read("Give a number:");
x = float(x);
sgn = try if(x>=0) return 1 else return -1;
print("Sign is "+str(sgn));
```

A similar syntax breaks away from loops below, though we will not dabble on 
handling returned values for now. Contrary to error handling overheads, 
tt is lightweight to intercept returns.

```java
// main.bb
counter = 0;
try while (true) {
    counter = counter + 1;
    print("Counter is: " + str(counter));
    if(counter==10)
        return;  // keeps exiting the code until intercepted by try
}
```

Finally, prepend `try` to loop bodies to let internal returns skip the rest of the body. 
All covered syntax allows at most one irregular exit point from control flows,
which makes code simpler.

```java
// main.bb
counter = 0;
while (counter<10) try {
    counter = counter + 1;
    if (counter % 2==0) 
        return;
    print("Odd value: " + str(counter));
}
```


## Missing values

Computations like converting invalid strings to numbers may 
generate missing values. These are different than errors in that they denote a completed algorithm 
that returns with nothing as an expected potential outcome. As a different example,
getting or setting out-of-bounds list element
creates errors, but the `next` operator of iterators returns a messing value once there are no
more items to iterate through.

Missing values also create errors, but only to the extend that the assignment
operator (`=`) prevents assigning such values to variables. In this case,
substitute the assignment with the `as` keyword. 
The differences are two: a) previous values are removed when overwritten with missing ones,
and b) this new kind of assignment yields a boolean value that indicates whether the set value 
was not missing.

For example, here is an a simple one-liner that retries
reading from the console until a number if provided:

```java
// main.bb
// the compiler understands `not` before `as` assignments to work on their output
while(not number as float(read("Give a number:"))) {}
print(number);
```

Or, equivalently based on function-based typecasting, which effectively chains
functions with only one argument on the argument before the `|` symbol.

```java
while(not number as "Give a number:"|read|float) {}
print(number);
```


```bash
> blombly main.bb
> Give a number: number
> Give a number: 12
12
```