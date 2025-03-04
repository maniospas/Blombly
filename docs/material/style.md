# Style guide

Here is a style guide that works well with Blombly's principles 
of simplicity and easy comprehension. 
**This style is optional but highly encouraged.**


## Spacing

Prefer placing spaces before and after lone `as`, `new`, and `=` symbols, to
cleanly separate what is being defined. It is better to avoid 
addiditional spaces around parentheses, curly, and square brackets.
Similarly, avoid spaces around the `|` symbol to make semi-type
chains feel like one value. Prefer adding a space after commas
of non-list elements, and after semicolons if multiple statements 
reside in the same line.
Do not forget to place a parentheses around lists when converting
them to vectors. Here is an example:

```java
// main.bb
A = vector(1,2,3);
print(A[2]);
```

## Semi-types

When you chain transformations try to have at least one but otherwise 
as few as possible parentheses in each statement. For example, in assignments
like the following it becomes obvious by reading the beginning of the assignment
that `n` is going to be a length that is retrieved through a chain of conversions
starting from the given string.

```java
// main.bb
n = len("What's your name?"|read);
print("Hi person with name length !{n}.");
```

## Brackets

When declaring code blocks, place the opening bracket in the same
line as the definition, and indent the following lines. Place the 
closing bracket in a separate line. Here is an example:

```java
// main.bb
adder(x, y) = {
    default bias = 0;
    return x + y + bias;
}
print(adder(1, 2));
```

If code blocks comprise only one statement, prefer
placing everything in one line. In this case, also forgo the
trailing semicolon for less symbol clutter.
That said, code blocks implemented in one line usually
just return a value, in which case use 
`=>` to reduce the amount of nesting.
Two equivalent examples follow.

```java
// main.bb (without applyign =>, which is not preferred)
fmt(x) = {return x[".3f"]}
number = "Give a number:"|read|float;
print("This is a number !{number:fmt}");
```

```java
// main.bb (preferred)
fmt(x) => x[".3f"];
number = "Give a number:"|read|float;
print("This is a number !{number:fmt}");
```

Similarly, prefer using control flow without brackets. When you do
so, prefer placing bracketless statements in the same line as their
controlling condition. The language's syntax is curated so
that only the trailing semicolon will appear in each line.
Here is an example:


```java
contains(A, num) = {
    while(a in A) if(a==num) return true;
    return false;
}

A = 1,2,3,4;
num = 3;
```

Use the gather macro to adopt a functional way of anonymizing code
while taking advantage of the control flows of imperative programming.
Use only *one* gather statement, and place any transformations inside.
Do not forget that, inside this macro, the current scope is fully visible,
whereas all yielded values are aggregated. An example follows.


```java
A = 1,2,3,4,5;
value = !gather(0, +=) {while(i in A) if(i%2==0) yield i^2;}
print("Sum of squares !{value}");
```


## Extensibility

Do not make variables final unless you want to expose
them to called blocks (usually, this is also needed
for recursion). Leaving everything in its default mutable
state helps set up an extensible environment.
Avoid module conflicts by placing include statements
within `new` declarations. Here is an example:

```java
final html = new {!include "libs/html"}
print(html.html); // this will tell you that this is a code block
```

**Prefer !local instead of !macro to redefine macros for
only one file.** In general, macros should be avoided
for anything that is not injecting metaprogramming patterns.


