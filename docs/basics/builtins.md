# Sequential code

This section covers Blombly's most basic commands, namely comments, variable handling, builtin datatypes,
and control flow. Some concepts may be familiar, but the reader is encouraged to skim through this section
as it also touches some unique language features.

## Comments

Blombly only has line comments that start with `//`. However, it supports multi-line strings and these can be set as specification metadata like in the snippet below. 
In this, the preprocessor instruction `#spec doc = ...` creates the `doc` specification for a whole code block or file. 
Specifications are only allowed at the beginning of code (so as the first instructions of your main file) and, once set, are immutable.

```java
// main.bb
// This is a line comment.
#spec doc = "This is multiline documentation 
           \nthat describes the current file."; 
print("Hello world!");
```

## Builtins

Assign values to variables with the equality operator (`=`), like below. If a variable with the same name already exists in the current scope, its value is overwritten. 
Otherwise, a new variable is created. Subsequent code in the same scope will retrieve that assigned value when using the variable. Scopes refer to isolated execution contexts 
in which subsequent code can overwrite variable values. Each program starts from one initial scope, but new ones are created when creating new objects or calling methods.

There are several builtin data types that are directly incorporated in the language.
Exhaustively, these are `int`, `float`, `bool`, `str`, `list`, `vector`, `code`, `struct`, `server`.
Here we start with the most basic ones, ans leave the last three for dedicated subsections.

```java
// main.bb
i = 1;     // int 
f = 1.2;   // float 
b = false; // bool (or true)
s = "this is a string literal";
a = 1,2,3;     // list
v = vector(5); // its number of elements
m = map();     // hash map
```

Some well-known operations on the above types are listed below. These are computed as one might have come to learn from other programming
languages.

| **Category**             | **Operation**                          | **Description**                                          |
|--------------------------|----------------------------------------|----------------------------------------------------------|
| **Arithmetics**          | `+`, `-`, `*`, `/` <br> `^` <br> `%`   | Basic arithmetics (division is floating-point). <br> Exponentiation. <br> Modulo for integers. |
| **Comparisons**          | `<`, `>`, `<=`, `>=` <br>  `==`, `!=`  | Inequality comparisons. <br> Equality and inequality comparisons. |
| **Boolean operations**   | `and`, `or` <br> `not`                 | Logical operations for booleans.  <br> Negation of any boolean value it prepends.|
| **String operations**    | `+`                                    | Concatenation.                                                    |
| **Convertion**           | Type names.                            | Everything can be converted from and to a string.                 |
| **Elements**             | `a[i]`, `a[i]=x`                       | Element get and set for strings, maps, lists, and vectors.       |


Here is an example:

```java
// main.bb
x = int("1"); 
y = float("0.5");
print("Sum is "+str(x+y)); // there is no implicit typecasting
```

```bash
> Blombly main.bb
Sum is 1.500000
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
and error handling. The first two of those are described here, whereas method calling is [next](blocks.md) in the menu because it offers more options than other languages. 
Error handling also has its own dedicated page [here](errors.md) but below we also give a first taste of the `try` keyword's dynamic usage.

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

## Sneak peek at signals

To get a full sense of how control flow may work with other commands,
here we briefly introduce signals - 
the mechanism behind Blombly's error handling. We use these to create the
equivalent of continue and break statements without needing more keywords.

In particular, the `value = try{@code}` intercepts both errors 
and `return value;`
statements on its internal code segment. Given that brackets may again
be ommited for a segment of one command, one can use it to compute
one-line conditions like in this example:

```java
// main.bb
x = read("Give a number:");
sgn = try if(x>=0) return 1 else return -1;
print("Sign is "+str(sgn));
```

Asimilar syntax can stop loops, though we will not dabble on handling returned values for now. 
What should be mentioned is that, contrary to errors, intercepting returns is lightweight.

```java
// main.bb
counter = 0;
try while (true) {
    print("Counter is: " + str(counter));
    counter = counter + 1;
    if(counter==10)
        return;
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

Some operations, such as returning with no value or converting invalid strings to numbers, generate missing values. These
are different than errors in that they denote a completed algorithm that returns with nothing as
an expected potential outcome. 

Assigning missing values to a variable is considered a logical error by the interpreter, and we denote that this is ok 
by using the `as` keyword in lieu of the assignment operator ('='). 
This new assignment operator is different in that it silently erases the previous value when the new one is missing. 
Furthermore, it yields a boolean value that indicates whether a new value was set. Here is an example that retries
reading from the console until a number if provided. The `as` operator lets use create a simple one-liner.

```java
// main.bb
// the compiler understands `not` before `as` assignments to work on their output
while(not number as float(read("Give a number:"))) {}
print(number);
```
```bash
> blombly main.bb
> Give a number: number
> Give a number: 12
12
```