# The preprocessor

Blombly's preprocessor can understand several instructions that transform the source code before compilation into `.bbvm` files. 
Preprocessor instractions are made distinct by prepending them with a `#` symbol.
Three main types of preprocessing are available: dependencies that make a source code file include another's code, 
macros that add to the language's grammar with higher-level expressions, and specification property declarations.

## Specifications

Specifications have already been described as a mechanism that attaches final values to code blocks. Evaluating them 
just after the block's definition.

## Dependencies

Dependencies on other `.bb` files or folders are stated with an include statement that looks like this `#include "std/oop"`.
When an include is encountered inlines the contents of the file `std/oop.bb` or, if `std/oop` is a folder of the filter `std/oop/.bb`. 
Dependencies allow code modularization without loading overheads, as the compilation outcome packs all necessary instructions to run itself. 
Dependencies should not declare specifications, as these are the provenance of code blocks meant to run dynamically instead of immediately upon import. 
When creating reusable libraries, prefer constructing modules and final objects (this is automated with the module keyword of `std/oop`).


## Macros

Macros are transformations for reducing boilerplate code. They are declared with statements of the form `#macro (@expression) = (@transformation);`
Both the expression and transformation parts consist of fixed "keyword" tokens and named wildcard tokens, prepended with att (`@`). 
Wildcards represent any sequence of tokens and matched between the expression and the transformation. 
To support faster compilation and improve comprehension, the first token of the expression needs to be a keyword (e.g., `fn @name (@args)` is valid).

Here is an example of how macros simplify code, defining a class and function with macros from the std/oop module.
Using these, one can now define classes, modules, and functions more concisely:

```java
// oop.bb
#macro (class @name {@code}) = (final @name = {@code});
#macro (fn @name(@args) {@code}) = (final @name(@args) = {@code});
#macro (module @name {@code}) = (final @name = new {@code});
```

```java
// main.bb
#include "oop.bb"

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

print(finder.next());
print(finder.next());
```

## The standard library

*This section is under construction.*


## Writing an new library

Below is an example of what a module may look like. Instead of specifications, `final` properties are set for documentation and versioning.
However, methods (which are essentially code blocks meant to be called) obtain specifications. 
Notice that those specifications can depend on the library's other properties. To use the module in your code, include the module and call the relevant methods.

```java
// main.bb
#include "mylib"

mylib.tests();
print(mylib.add(1, 2));
```

```java
// mylib.bb
#include "libs/oop"
enable oop;

module mylib {
    final name = "mylib";
    final author = "Your Name";
    final doc = "This is a simple numeric interface";
    final version = 1.0;

    fn add(x, y) {
        #spec doc = "Computes an addition in " + name;
        #spec version = version;
        return x + y;
    }

    fn abs(x) {
        #spec doc = "Computes the absolute value in " + name;
        #spec version = version;
        if (x < 0) return -x;
        return x;
    }

    final tests = {
        if (add(1, 2) != 3) 
            fail("Invalid addition");
        if (abs(-1) != 1) 
            fail("Invalid abs");
        print("Tests successful.");
    }
}
```
