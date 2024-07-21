# Guide for Blombly

**1 Set things up**<br>
1.1 [Hello world!](#11-hello-world)<br>
1.2 [Errors](#12-errors)<br>

**2 Language features**<br>
2.1 [Vars, operations, comments](#21-vars-operations-comments)<br>
2.2 [Code blocks & inline](#22-code-blocks--inline)<br>
2.3 [Calling blocks](#23-calling-blocks)<br>
2.4 [Structs](#24-structs)<br>

# 1 Set things up

## 1.1 Hello world!

Let us create a first program to show a compilation 
and interpretation process. Create an empty text file
`main.bb` with the following contents:

```java
print("Hello world!");
```

Careful to not forget the trailing semicolon. In general,
use a Java syntax highlighter (not a syntax checker), 
as it shares many keywords. To run this file, download 
blombly's executable from the [downloads](./downloads.md) 
and add it to the filepath. Run `blombly main.bb` to 
compile and run it. This will create the following output 
in the console:

```
 ( OK ) Compilation
 ( OK ) Optimization
Hello world!
```

Compilation creates an intermediate representation 
to the language's virtual machine in the file `main.bbvm`.
It also applies some optimizations for faster execution. 
You can directly run `.bbvm` files to avoid compiling and 
optimizing again.

:bulb: The script `bbreader.py` converts compiled code
to the main language.


## 1.2 Errors

Before moving on, let us take a peek at errors
that blombly may create. Broadly, there are syntax errors,
which are identified by the compiler and make it halt,
and logical errors that occur at runtime and can be caught
with appropriate try clauses (see later). To see what a
syntax error looks like, let us try to execute the
following invalid code:

```java
print("Hello"+);  // CREATES AN ERROR
```

We get the following error, from which we can see that
the `+` operation for string concatenation (see later)
is added with no right-hand side. Notice that the 
compiler shows the full deconstruction process into
an abstract syntax tree, to help understand what it's
doing.

```
 ( ERROR ) Empty expression
   →                                             main.bb line 1
   → "Hello" +                                   main.bb line 1
   → ( "Hello" + )                               main.bb line 1
   → print ( "Hello" + )                         main.bb line 1
   → print ( "Hello" + ) ;                       main.bb line 1
```

Let us now see what a logical error looks like too. Try
to print a variable that does not exist in the following code:

```java
print(x);  // CREATES AN ERROR
```

This creates the following error during runtime. It points
to the stack trace of both compiled code and the original
source. If the source file is missing, you will still get 
to see where the error's lines, for example to make sense
of stack traces reported by others.

```
 ( OK ) Compilation
 ( OK ) Optimization
 ( ERROR ) Missing value: x
   → print # x                                   main.bbvm line 1
```

If the error was more complicated, the virtual machine would
show the full stack trace leading to the failure. Errors maintain
a full stack trace even if segments of the code run in threads.

:bulb: There are metadata properties that can obfuscate code
and error tracking.

# 2 Language features

## 2.1 Vars, operations, comments

Blombly only has line comments that start `//` and
stop at the end of the current line. Set variables 
by assinging to them, like below. Builtin types are
int, float, bool, str, struct, code. Here is an example 
with the first four, where the other two will be covered
later (structs can also customly implement operations).

```java
i = 1;     // int
f = 1.2;   // float
b = false; // bool
s = "this is a string literal";
```

The numeric types int and float accept the typical
arithmetics `+-*/`, the exponent `^`, modulo between 
ints `%`, and inequalities with bool outcomes `< > <= >=`.
Comparisons `== !=` can compare any types. 
Operations for bools are `and` and `or`, and str can
be concatenated with `+`.

Convert between types with their names like so:

```java
x = int("1");
y = float("0.5");
print("Sum is "+str(x+y));
```

Running this creates the following output in the console:

```
 ( OK ) Compilation
 ( OK ) Optimization
Result is 1.500000
```

Lastly, variables can be made immutable by prepending
the `final` keyword to the assignment. This prevents
any overwritting of their value and exposes them outside
the current scope, for example in dependent code blocks
calls later. Here is an example where there is an attempt
to set a final variable:

```java
final x = 0;
x = x+1;  // CREATES AN ERROR
```

When running the above, the code safety of the
final keyword lets us create an error message
instead of erroneously overwritting the value:

```
 ( OK ) Compilation
 ( OK ) Optimization
 ( ERROR ) Cannot overwrite final value: x
   → add x x _bb2                                main.bbvm line 4
```

## 2.2 Code blocks & inline

The main logic compartmenization mechnism in blombly
are code blocks; these can be treated as methods, used
to define various control flows, or called inline. Here
we start with basic code block definition and inline 
execution.

Code blocks are declared by enclosing them in brackets
and then assigning them to a variable, like below. 
Notice that there is no trailing semicolon (the
compiler will create an error for you if you try
to add one). The declaration can also be `final`.

```java
test = {
  print("Hellow world");
}
```

The declaration iself does not run any code. You 
can effectively "paste" a block's internal code
to the current position by performing inline execution
like below. The expression ends with double colon `:`
instead of a semicolon to perform inlining on its result:

```java
test:
```

We will tackle usage of code blocks as methods in the next
segment. For now, we focus on how to also declare and
retrieve block metadata with the dot `.` notation like 
structs/objects in most languages. Block metadata are set
as immutable values, and thus you need to prepend the
`final` keyword or an error will occur. Here is an example
of setting block metadata.

```java
final test = { // ensure that this does not change in current scope
  print("Hello world! I am a running code block.");
}
final test.version = 1.0;  // setting metadata is always final
final test.license = "CC0";

print("Running test version "+str(test.version)
     +" under "+test.license+" license...");

test: // block code directly runs here
```

## 2.3 Calling blocks



## 2.4 Structs

You can create a data structure by using the `new {...}` syntax, 
where the interior of the code blocks. 


:warning: To prevent a wide range of code smells, the compiler
does not allow you to write `new B` where `B` is a code block. 
Instead, inline it.
