# The preprocessor

Blombly's preprocessor can understand several instructions that transform the source code before compilation into `.bbvm` files. 
Preprocessor instractions are made distinct by prepending them with a `#` symbol.
Three main types of preprocessing are available: dependencies that make a source code file include another's code, 
macros that enrich the language's grammar with higher-level expressions, and other supporting code transformations.

**In practical code writting, you will mostly use dependencies.** Macros and other transformations may alter
the order in which written code is executed. They should be sparingly during development - if at all. Ideally,
they are meant to support certain libraries that inject new coding patterns, like those found [here](libraries.md). 
Most new libraries are not expected to use these features either.

## #include

Dependencies on other `.bb` files or folders are stated with an include statement that looks like so:

```java
#include "libs/loop"
```

When an include statement is encountered inlines the contents of the included path if a suffix `.bb` or `/.bb` is added.
In the above example, this means that either `libs/loop.bb` is included if it exists or, if `libs/loop` is a folder, `libs/oop/.bb` is included.

Dependencies enable code modularization without loading overheads, as the compilation outcome packs all necessary instructions to run 
automously by the interpreter. Dependencies should not declare specifications, as these are the provenance of code blocks meant to run 
dynamically instead of immediately upon import. 
When creating reusable libraries, prefer packing some instructive metadata in some commonly accepted format. One such 
format is presented in the [next section](libraries.md).


## #macro

Macros are transformations for reducing boilerplate code. They are declared with statements of the form `#macro {@expression} as {@transformation}`
Both the expression and transformation parts consist of fixed "keyword" tokens and named wildcard tokens, prepended with att (`@`). 
Wildcards represent any sequence of tokens and are matched between the expression and the transformation. 

To support faster compilation, improve comprehension, and completely the inherent ambiguity that mixfit operators may create,
the first token of the expression needs to be a keyword (e.g., `fn @name (@args)` is valid). Then, macros are always applied
based on order of occurence.

Here is an example of how macros alter code writting by defining a simpler version of some found in `libs/oop`:

```java
// oop.bb
#macro {class @name {@code}} as {final @name = {@code}}
#macro {fn @name(@args) {@code}} as {final @name(@args) = {@code}}
#macro {module @name {@code}} as {final @name = new {@code}}
```

```java
// main.bb
#include "oop"

class Finder { 
    fn contains(number) {
        i = 2;
        i_max = int(number^0.5);
        while (i <= i_max) {
            if (number % i == 0) return false;
            i = i + 1;
        }
        return true;
    }

    fn \next() {
        while (true) {
            this.number = this.number + 1;
            if (this.contains(number)) return this.number;
        }
    }
}

finder = new {Finder: number = 10;}

print(next(finder));
print(next(finder));
```

## #of

The first code transformation we will look at is the `#of` statement.
This is prepended at the beginning of a parenthesis to assign
everything inside to a variable just before the last semicolon `;`
at the same nested level or -if that is not found- at the beginning
of the current code block. 

An example and its equivalent 
implementation without the preprocessor are presented below. The
difference is that, in the first case, an anonymous variable (starting with
the `_bb` prefix) is internally created instead of `it` to hold the
iterator. That variable replaces the contents of the `#of` parenthesis.

```java
// main.bb
A = 1,2,3;
while(x as next(#of iter(A)))
    print(x);
```

```java
// main.bb (equivalent code)
A = 1,2,3;
it = iter(A);
while(x as next(it))
    print(x);
```

To properly write loops that traverse all the elements of an iterable,
look at the `loop` library [here](libraries.md).

## #stringify

This converts a sequence of keywords and strings separated by spaces
into one larger string. This operation is executed *at compile time*.
This is especially convenient for converting macro variable names into
strings. Here is an example:

```java
// main.bb
message = #stringify(Hello " world!");
print(message);
```

```bash
> blombly main.bb
Hello world!
```

## #symbol

This is similar to `#stringify` with the difference that at the end
it converts the sequence of tokens into a source code symbol. Note that,
by being performed at compile time, this does not (and cannot) take into 
account any values associated with the symbol parts. Again, this is 
conventient for macro definitions that construct variable names based
on other variable names. Here is an example:

```java
// main.bb
#symbol(var name) = "Hello world!";
print(varname);
```

## #fail

This immediately fails the compilation process upon occurence. This
stringifies the next symbol (or leaves it intact if it is a string)
and immediately exits the compilation process. This is often used
by libraries defining macros with overlap with the standard functionality
to prevent ambiguous symbols from being used. Here is an example:

```java
// main.bb
#macro {next} as {#fail "use std::next instead"}  // prevent usage of next
A = 1,2,3;
while(x in next(A))
    print(x);
```

```bash
> blombly main.bb
( ERROR ) use std::next instead 
→   while(x in # fail "use std::next instead"(A))  main.bb line 3
    ~~~~~~~~~~~^
→   while(x in # fail "use std::next instead"(A))  main.bb line 3
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
```