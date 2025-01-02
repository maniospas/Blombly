# The preprocessor

Blombly's preprocessor can understand several directives that transform the source code before compilation into `.bbvm` files. 
These directives are made distinct by prepending them with a `!` symbol.
Three main types of preprocessing are available: dependencies that make a source code file include another's code, 
macros that enrich the language's grammar with higher-level expressions, and other supporting code transformations.

**In practical code writting, you will mostly use dependencies.** Macros and other transformations may alter
the order in which written code is executed. They should be sparingly during development - if at all. 
They are meant to support libraries that inject new coding patterns, like those found in the `libs/.bb` file
that the compiler includes before anything else so as to create some useful idioms.

## !include

Dependencies on other `.bb` files or folders are stated with an include statement that looks like so:

```java
!include "libs/html"
```

When an include directive is encountered, it inlines the contents of the next path if a suffix `.bb` or `/.bb` is added.
This happens only if the path has not already been included, which allows for circular inclusions.
In the above example, this means that either `libs/html.bb` is included or, 
if `libs/html` is a folder, `libs/html/.bb` is included. Inclusion paths are checked both relatively to blombly's
executable and to the main file beeing compiled.

Dependencies enable code modularization without loading overheads, as the compilation outcome packs all necessary instructions to run 
automously by the interpreter.


## !macro

Macros are transformations for reducing boilerplate code. They are declared with statements of the form `!macro {@expression} as {@transformation}`
Both the expression and transformation parts consist of fixed "keyword" tokens and named wildcard tokens. Wildcards are prepended with att (`@`). 
and match any sequence of tokens. If you define a macro within
another macro use two att symbols as the wildcard's 
prefix (e.g., `@@metavariable`). In this case, only the beginning `@` is removed.
To define a macro within the nested macro use three att symbols, and so on.

To support faster compilation, improve comprehension, and avoid the inherent ambiguity that mixfit operators may create,
the first token of the expression needs to be a keyword (e.g., `fn @name (@args)` is a valid definition, but `@name = fn (@args)` is not)
Macros are always applied based on order of occurence, with the last applicable one taking precedence.

Next is an example of how macros can be used to alter code writting. This is very intrusive to the language and is not really recommended,
but you can use things like this to customize the language to your tastes, effectively creating a variant.

```java
// oop.bb
!macro {class @name {@code}} as {final @name = {@code}}
!macro {fn @name(@args) {@code}} as {final @name(@args) = {@code}}
!macro {module @name {@code}} as {final @name = new {@code}}
```

```java
// main.bb
!include "oop"

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

    fn next() {
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

## !of

The first code transformation we will look at is the `!of` statement.
This is prepended at the beginning of a parenthesis to assign
everything inside to a variable just before the last semicolon `;`
at the same nested level or -if that is not found- at the beginning
of the current code block. 

An example and its equivalent 
implementation without the preprocessor are presented below. The
difference is that, in the first case, an anonymous variable (starting with
the `_bb` prefix) is internally created instead of `it` to hold the
iterator. That variable replaces the contents of the `!of` parenthesis.

```java
// main.bb
A = 1,2,3;
while(x as next(!of iter(A))) print(x);
```

```java
// main.bb (equivalent code)
A = 1,2,3;
it = iter(A);
while(x as next(it)) print(x);
```

```bash
> ./blombly main.bb
1
2
3
```

Blombly's `in` macro that is shipped with the language
wrappes this exact behavior.


## !stringify

This converts a sequence of keywords and strings separated by spaces
into one larger string. This operation is executed *at compile time*
and is especially convenient for converting macro variable names into
strings. Here is an example:

```java
// main.bb
message = !stringify(Hello " world!");
print(message);
```

```bash
> ./blombly main.bb
Hello world!
```

## !symbol

This is similar to `!stringify` with the difference that at the end
it converts the sequence of tokens into a source code symbol. 
Due to being performed at compile time, this does not (and cannot) take into 
account any values associated with the symbol parts. Again, this is 
conventient for macro definitions that construct variable names based
on other variable names. Here is an example:

```java
// main.bb
!symbol(var name) = "Hello world!";
print(varname);
```

## !fail

This immediately fails the compilation process upon occurence. It
stringifies the next symbol (or leaves it intact if it is a string)
to create an error message and may be used
by libraries defining macros with overlap with the standard functionality
to prevent ambiguous symbols from being used. Here is an example:

```java
// main.bb
!macro {next} as {!fail "use bbvm::next instead"}  // prevent usage of `next` from now on
A = 1,2,3;
while(x as next(A)) print(x);
```

```bash
> ./blombly main.bb
( ERROR ) use std::next instead 
→   while(x as !fail "use bbvm::next instead"(A))  main.bb line 3
    ~~~~~~~~~~~^
→   while(x as !fail "use bbvm::next instead"(A))  main.bb line 3
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^
```
