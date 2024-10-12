# Prepacked libraries

Blombly is shipped alongside a couple of small libraries that
you can use out-of-the-box to simplify some aspects of development 
and speed up code writting.

## env

This is the most important library that provides an alternative
to some import statements that helps organize versioning and 
dependencies. It essentially forms a programmatically-managed
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
for(x as loop::next(A))
    print(x);

env::versions();
env::help("loop");
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
loop 1.0.0
Copyright (C) 2024, Emmanouil Krasanakis
Apache 2.0 license
     
 Introduces the `loop::next` macro that allows     
 iteration over all elements of a list or other     
 iterable, like so:     
     
    A = 1,2,3;     
    while(x as loop::next(A))      
       print(x);

 This is a wrapper around `std::next` that creates
 an iterator for the supplied expression just
 before the loop's command.
-------------------------------------------------------------
```

The above example is a variation of the preprocessor
inlcude statement with some simple reporting mixed in.
However, you can also require a version and minimum release
like below. Notice the intentional similarity to method calls, 
despite the include statement being a macro.

```java
env::include("loop"|version="1.0";minrelease=0);
```
