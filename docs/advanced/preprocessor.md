# Preprocessing

Blombly's preprocessor can understand several directives that transform the source code before compilation into `.bbvm` files. 
These directives are made distinct by prepending them with a `!` symbol.
Five main types of preprocessing are available: dependencies that make a source code file include another's code, 
expressions to be evaluated at compile time, permissions to read and write system resources, string interpolation,
and macros that enrich the language's grammar with higher-level expressions.

<br>

Macros may alter the order in which written code is executed 
and should be sparingly used - if at all. Therefore, the macro and related direactives are 
packed into one section in this page. Macros are meant
to support coding patterns without needing to explicitly code them in
the virtual machine.

## !include

Dependencies on other `.bb` files or folders are stated with an include statement that looks like so:

```java
!include "libs/html"
```

When an include directive is encountered, it inlines the contents of the next path if a suffix `.bb` or `/.bb` is added.
This happens only if the path has not already been included, which allows for circular inclusions.
In the above example, this means that either `libs/html.bb` is included or, 
if `libs/html` is a folder, `libs/html/.bb` is included. Inclusion paths are checked both relatively to blombly's
executable and to the main file being compiled.
Dependencies enable code modularization without loading overheads, as the compilation outcome packs all necessary instructions to run 
automously by the interpreter.

## !comptime

This directive evaluates expressions at compile time, and then packs their outcome in the produced intermediate
representation. For example, consider the next code open that opens web page as a file, obtains its contents with conversion to
str, obtains the latter's length, and finally prints the result. Make the whole process run at `!comptime` and 
look at what the produced intermediate representation looks like: it contains only the precomputed value. 
This directive accepts any blombly expression, including those that include other comptimes. 
To avoid removal of the return value due to optimizations run code blocks inside it with `do`.
If you do not plan to retrieve a value, just put a code block next to it, or write a single bracketless command.

```java
// main.bb
!access "http://" // grant access permissions to http requests
googlelen = !comptime "http://www.google.com/"|file|str|len;
print(googlelen);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
55079
> <span style="color: cyan;">cat</span> main.bbvm
BUILTIN googlelen I55079
print # googlelen
</pre>



!!! info
    Each comptime is executed independentently from the rest of compilation, although it
    inherits its permissions (it cannot set its own). If a return value is insufficient,
    it can also exchange information through permmited resources, such as the virtual 
    file system.

## Permissions

Blombly's environment restricts itself with regards to which resources it can access.
By default, it only access and modify RAM, threads, the console's input and output,
and a virtual file system. It can also access the file system of your working directory (only!)
and cannot modify any file in your system or web resource whatsoever. Doing so must be expressly 
allowed with permissions stated in the
main file. By *main file* we refer to the one that is passed as an argument
to Blombly to compile and run.

<br>

There are two kinds of permissions: accessing resources with `!access` and modifying
resources with `!modify`. Modifications imply access too, though this may change 
in the future. Both permission directives are followed by a string literal that determines 
prefixes of resources. You can have multiple permission statements, which work cumulatively 
throughout the same Blombly virtual machine.
For safety, that literal cannot be determined by comptime. For more details about
permission usage and the various types of resources, look at the [IO](../basics/io.md)
page.

<br>

In more complicated projects, 
permissions stated in files other than the main one only help by creating
pre-emptive error messages during compilation. They should, again,
be actually allowed from the main file.
Comptime is also subjected to your main file's permissions.
Finally, previously compiled *.bbvm* files contain NO permissions.
In those cases, provide permissions by running more main files.
Recall that you can provide code to run instead of files
in the language's command line arguments. For example, below
we compile a *main.bb* file and then run the compiled file while 
granting it *https://* access permissions.

```java
// main.bb
print("http://www.google.com/"|file|str|len);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --norun
> <span style="color: cyan;">./blombly</span> '!access "https://' main.bbvm
55079
</pre>


!!! warning
    If you compile or run multiple main files with the same virtual machine, permissions 
    and the virtual file system carry over. This is useful for declarative
    build configurations, but you need to know about it to not arbitrarily
    add stuff from others to your files.



## String interpolation

String interpolation is performed by enclosing a part of strings in `!{...}`.
This splits the string on compile time to calling `str` on the enclosed epxression
and performing string concatenation with the string segments to the left and right.
For example, the two print statements in following code segments are identical:

```java
name = read("what's your name?");
print("Hi !{name}.");
print("Hi "+str(name)+".");
```

## Namespaces

Namespaces compartmenize the usage of certain variables.
THis is done adding a prefix `@name::` to their names when the namespace is not active,
which makes it harder to use by mistake.
The name is added as a prefix followed by two colons
to only a set of affected variables following the namespace's activation,
and until the end of the file.
To begin with, declare a namespace with the following syntax,
where at its end you can also add some code to run upon
activation. Include any number of variables whose
subsequent usage is altered.

```java
namespace @name {
    var @v1;
    var @v2; 
    ...
}
```

Similarly to code blocks, declaring a namespace does nothing.
But you can bring it in the with the syntax `with @name:`. Notice
the colon at the end, which is intentionally similar to code blocks
to indicate that subsequent code is affected. 
Refer to a variable affected by a namespace
either through its full name (e.g., `graphics::x`) or by first activating
the corresponding namespace. Below is an example of
using namespaces to differentiate between usage of the same
variables.

```java
// main.bb
namespace dims {
    var x;
    var y;
}
namespace main {
    var x;
    var y;
}

with dims: // x and y are now dims::x and dims::y 
Point = {
    norm() => (this.x^2+this.y^2)^0.5;
    str() => "(!{this.x}, !{this.y})";
}
p = new {Point: x=3;y=4}

with main:
p.x = 0;
print(p);
print(p.main::x);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(3,4)
0
</pre>


!!! tip
    Namespace usage is encouraged. They are better than a simple zero cost abstraction (they are
    implemented with macros without any cost) that helps with debugging
    in that thet help the virtual machine better reason about how to parallelize programs.

## Macros

**!macro**

Macros are transformations for reducing boilerplate code. They are declared with statements of the form `!macro {@expression} as {@transformation}`
Both the expression and transformation parts consist of fixed "keyword" tokens and named wildcard tokens. Wildcards are prepended with att (`@`). 
and match any sequence of tokens. If you define a macro within
another macro use two att symbols as the wildcard's 
prefix (e.g., `@@metavariable`). In this case, only the beginning `@` is removed.
To define a macro within the nested macro use three att symbols, and so on.

<br>

To support faster compilation, improve comprehension, and avoid the inherent ambiguity that mixfit operators may create,
the first token of the expression needs to be a keyword (e.g., `fn @name (@args)` is a valid definition, but `@name = fn (@args)` is not)
Macros are always applied based on order of occurrence, with the last applicable one taking precedence.
Next is an example of how macros can be used to alter code writing. This is intrusive to the language and not really recommended,
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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
11
13
</pre>

The following directives play a supporting role to other language features.

<br>

**!local**

Make macros valid only for the length of the current source code file by declaring
them as `!local {@expression} as {@transformation}`. The same rules as above
hold, where you can interweave local definitions into macros and conversely.

!!! info
    Namespace variables are also implemented with `!local` under the hood
    to make sure that namespaces are explicitly re-enabled in different files.

<br>

**!of**

The first code transformation we will look at is the `!of` statement.
This is prepended at the beginning of a parenthesis to assign
everything inside to a variable just before the last semicolon `;`
at the same nested level or -if that is not found- at the beginning
of the current code block. 
An example is presented below. There, an anonymous variable (starting with
the `_bb` prefix) is internally created instead of `it` to hold the
iterator. That variable replaces the contents of the `!of` parenthesis.
Blombly's `in` macro that is shipped with the language
wraps this behavior under the hood.

```java
// main.bb
A = 1,2,3;
while(x as next(!of A|iter)) print(x);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
2
3
</pre>



**!stringify**

This converts a sequence of keywords and strings separated by spaces
into one larger string. This operation is executed *at compile time*
and is especially convenient for converting macro variable names into
strings. Here is an example:

```java
// main.bb
message = !stringify(Hello " world!");
print(message);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>

**!symbol**

This is similar to `!stringify` with the difference that at the end
it converts the sequence of tokens into a source code symbol. 
Due to being performed at compile time, this does not (and cannot) take into 
account any values associated with the symbol parts. Again, this is 
convenient for macro definitions that construct variable names based
on other variable names. Here is an example:

```java
// main.bb
!symbol(var name) = "Hello world!";
print(varname);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>

**!fail**

This immediately fails the compilation process upon occurrence. It
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
