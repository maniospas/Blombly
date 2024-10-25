# Prepacked libraries

Blombly is shipped alongside a few small libraries that
you can use out-of-the-box to simplify some aspects of 
development and speed up code writting. These are
detailed here.

## env

This library provides an alternative to traditional import statements
helps organizes and versions dependencies. 
It essentially forms a programmatically-managed
virtual environment that manages libraries.

Under this management system, a library with name `@lib` should 
always reside in the path `libs/@lib` and contain the following
configuration, where its name is accurarelt reflected. By convention, the
version is a string that identifies a unique programmatic interface
and the release number an integer that is incremented on bug fixes that 
do not affect the interface.

```java
final @lib::INFO as {
    name    = "@lib";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "Description";
}
```

After including the environment management library, you can
then load subsequent libraries like below. You can use normal
includes for files within each library, but dependencies between
libraries should also use the environment's include. The manager
also allows you to view library information, where 
the version is in the format `"{version}.{release}"`.

```java
// main.bb
#include "libs/env"  // import the manager
env::include("loop");

A = 1,2;
while(x as loop::next(A))
    print(x);

env::versions();
env::help("env");
```

```plaintext
> blombly main.bb
1 
2 
-------------------------------------------------------------
Library              Version
env                  1.0.0
loop                 1.0.0
------------------------------------------------------------- 
-------------------------------------------------------------
env 1.0.0
Copyright (C) 2024, Emmanouil Krasanakis
Apache 2.0 license

 Introduces version control and documentation
 capabilities for other libraries. In particular,
 new libraries should contain a library::INFO
 code block that sets the following variables:
 name, author, license, version, release, year.
 Version control substitutes the import statement
 and can track imported libraries. Available
 macros follow.

 - env::include(library);
   Includes a library by its name (as a string).

 - env::include(library|version=...;minrelease=...);
   Includes a library with a specific version and
   minimum release number. You may ommit the latter.

 - env::help(library);
   Prints the details of the library.

 - env::versions();
   Prints a summary of all libraries introduced
   through `env::include` statements.
-------------------------------------------------------------
```

The above-described inclusion is a variation of the 
`#include` preprocessor statement with some simple reporting mixed in.
However, as you can already see from the library's own documentation,
you can also require a version and minimum release like below. 
Notice the intentional similarity to method calls, 
despite the include statement being a macro.

```java
env::include("loop"|version="1.0";minrelease=0);
```

## loop

This library provides methods to simplify list creation from iterables,
as well as a couple of alternatives for constructing
nameless iterators within loop bodies without naming them or 
needing additional commands. It achieves this
by employing the `#of` preprocessor statement under the hood.

#### Nameless iterators

The first type of iterator is one that is silently constructed
to go through the elements of an iterable. This results to a syntax 
like below, which is similar to for/foreach loops of other languages.
At the same time, this syntax introduces minimal new keywords, namely
the `loop::` prefix before usage of `next`.

```java
// main.bb
#include "libs/loop"  // or use env::include("loop");
A = 1,2,3;
while(x as loop::next(A)) 
    print(x);
```

A second iterator is for generating ranges of numbers and traversing
through them with (using `next` under the hood). Like so:

```java
// main.bb
#include "libs/loop"
while(i as loop::range(1, 5))
    print(i);
```


Both iterators invalidate the simple `next` and `range` keywords,
forcing the developer to choose their behavior with the `std::` 
or `loop::` prefixes that they normally alias.
The error message generated in the stead of these keywords explains 
the reasoning.

#### List generation

Moving on to list generation, one may pack all the elements of
an iterable into a list via the `loop::tolist` pattern macro.
In the simplest case, this is used as follows:

```java
// main.bb
#include "libs/loop"
it = std::range(5);
A = loop::tolist(is);
print(A);
```

```bash
>blombly main.bb
[0, 1, 2, 3, 4]
``` 

The next section introduces [semi-types](semitypes.md) as 
a notation that uses callables to transform values. As a preview,
the main notation `var|func` is equivalent to calling `func(var)`.
A similar notation can be used to create a callable to transform
all iterable elements before adding them to the generated list.
Here is an example:

```java
// main.bb
#include "libs/loop"
it = std::range(5);
A = loop::tolist(is|float);  // convert all elements to float
print(A);
```

Transformations that output missing values are skipped.
This way, you can values like so:

```java
// main.bb
#include "libs/loop"
A = loop::tolist("test", 1, 2|int);  // applies |int on all elements
print(A);
```

```bash
>blombly main.bb
[1, 2]
```


#### Lambda semi-types

For further convenience, one may create a nameless method
-also known as a lambda method- to perform the conversion.
This is simplified for practical usage and follows the 
pattern `loop::tolist(@iterable | @element->@expression)`.
The expression is *not* a code block but only some simple
calculation (though you can still use `new` to start a complex
computation). Here is an example:

```java
// main.bb
#include "libs/loop"
it = std::range(5);
A = loop::tolist(is|x->"val {x}");
print(A);
```

```bash
>blombly main.bb
[val 0, val 1, val 2, val 3, val 4]
``` 


The lambda notation above works only within the list convertion
mechanism. However, lambdas are also convenient for one-liner method 
definitions. In those cases, create them inside normal code with the
expression `symb::lambda(@variable -> @expression)`. Here is an example:

```java
// main.bb
#include "libs/loop"
#include "libs/symb"  // allows λ as a shorthand for symb::lambda
adder = λ(x,y->x+y);
print(adder(1,2));
```

Do not forget that `try` intercepts returns from
conditions or obtain missing values otherwise.
Here is an example that filters values based on
this pattern:

```java
// main.bb
#include "libs/loop"
A = 15, 1, 2;
A = loop::tolist(A|x->try if(x<10) return x);
print(A);
```

```bash
>blombly main.bb
[1, 2]
```
