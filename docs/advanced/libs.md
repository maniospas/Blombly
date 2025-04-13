# Libs
Blombly is packed alongside a *libs* directory that contains several out-of-the-box implementations.
This directory is included by default for all running instances, including spawned `!comptime` directives.
Its contents define several useful macros that have already been covered, as well as final `bb` struct
with many helper functions. Here we exhaustively describe available functionality.

## bb.ansi

Contains several ansi color codes under the `colors` namespace that you can use to colorize strings:
`black`, `lightblack`, `red`, `lightred`, `green`, `lightgreen`, `yellow`, `lightyello`, `blue`, `lightblue`, `purple`, `lightpurple`, `cyan`, `lightcyan`, `white`, `lightwhite`, `reset`.

Here is an example, where the namespace is enabled to access variable names:

```java
// main.bb
with colors:
print(bb.ansi.cyan+" INFO "+bb.ansi.reset+"This is a message.");
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
> <span style="color: cyan;">INFO</span> This is a message.
</pre>

## bb.collection

Contains helper methods for list and map manipulation using the currying notation. 
Here is how to push to a list (or struct with the corresponding overloaded operation):

```java
// main.bb
A = 1,2,3;
A |= bb.collection.toback(4);
print(A);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1,2,3,4
</pre>

## bb.logger

Provides a standardized logging interface for messages. It implements the methods `bb.ansi.log.fail(text)`,
`bb.ansi.log.info(text)`, `bb.ansi.log.warn(text)`, `bb.ansi.log.ok(text)`, which use ANSI to help colorize
message. 

## bb.memory

Provides memory management options that supplement the default reference counting mechanism.
Reference counting automatically clears any dangling memory, but does not explicitly handle
cycles across struct references. For example, when the following snippet terminates, Blombly
complains that there are two leaked memory contexts - tied to the respective structs `A` and `B`.

```java
// main.bb
A = new{message="Hello world!"}
B = new{A=A}
A.B = B;
print(A.message);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
(<span style="color: red;"> ERROR </span>) There are 2 leftover memory contexts leaked
</pre>


To break cycles like this, it suffices to remove one reference, for example by setting it
to something else, like `A.B=false`. Or, to be safe, you can completely clear the context's
internal variables to avoid keeping track of which exact fields. In the following snippet,
we defer clearing one of the structs until the end of the parent scope. Clearing
removes all data from a struct, but it is memory safe in that future access attempts create
an error that can be handled.

```java
// main.bb
A = new{message="Hello world!"}
defer clear(A); // there will be no leaks
B = new{A=A}
A.B = B;
print(A.message);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>


To properly handle cyclic references in complex applications, create a
`bb.memory.raii()` struct to hold a shared heap space. Push on this various structs
to be automatically cleared. Below is an example where we
again defer clearing to the end. The push operation returns the object
being added in the memory.

```java
// main.bb
memory = bb.memory.raii();
defer clear(memory);

A = memory<<new{message="Hello world!"}
B = memory<<new{A=A} // add at least one object in the memory to break the cyclic reference.
A.B = B;
print(A.message);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>

## bb.os

Automates several file handling tasks. In addition to those operations,
filesystem access safety is imposed with the `!access` and `!modify` preprocessor
directives described [here](../basics/io.md). Access any system resources without explicitly
providing the respective permissions to your program *from your main/running file*.
For safety, required permissions found in any included files that are not a subset of those
declared in the main program create an error and ask to be added.


```java
!access "https://raw.githubusercontent.com/"
!modify "libs/download/"

bb.os.transfer("libs/download/html.bb", "https://raw.githubusercontent.com/maniospas/Blombly/refs/heads/main/libs/html.bb");
```

## bb.sci

Provides a small data science package with plotting functionality over vectors.
Plotting uses `graphics` under the hood. Here is an example.

```java
x = list();
y = list();
while(i in range(100)){
    x << i*0.01;
    y << ((i-50)*0.01)^2;
}
canvas = new{bb.sci.Plot:}
canvas = canvas.plot(x, y :: color=255,0,0,255; title="y=(x-0.5)^2");
canvas = canvas.plot(x, x :: color=0,255,0,255; title="y=x");
canvas.show(width=800;height=600);
```

## bb.flat

Blombly’s design intentionally omits types for structs to stay flexible. 
However, in practice, bundling a few related values together 
is a common need when passing data between interfaces. 
Inspired by languages like Zig, *bb.flat* offers a way to bind small 
groups of values efficiently and have them look similar to structs
without actually creating structs.

<br>

This is a zero-cost abstraction that allows you to name a sequence 
of comma-separated elements. Everything is resolved into local varaibles
during compilation, eliminating most allocations.

<br>

Flat data types are declared using the `!flat` directive, followed by a name
and argument names in a parenthessis - like a function signature without a body.
To create variables of those types, prepend the assignment with the flat type’s name
or put the type name before variable names in other types or function signatures.
Once declared, those variables are automatically unpacked into individual elements, 
which can then be referenced using dot-notation (e.g., `p.x`, `p.y`) directly 
in the intermediate representation. Types are retained until the end of file,
so use `!include{...}` to scope them. Use namespaces to borrow flat types.

<br>

Flat variables are passed by value to functions. Conceptually, they treat memory 
like a list of values, and the compiler does use this format to store them in the
heap, for example in objects, lists, or maps. In the example bellow, the
signature `(Point a, Point b)` is unpacked to `(a.x, a.y, b.x, b.y)` in the example below, 
where each one is treated as one name
with the dot embedded insde instead of composite objects. This interoperability
with normal arguments allows you to directly use comma-separated values when
flats are expected.

<br>

Similarly, flat assignment syntax (e.g., `Point p = 1,2;`) ensures simple expansion 
to the underlying primitive types. Assignments like `p2 = p1` merely copy 
lists of values without introducing tuple-specific metadata. But the compiler
optimizes away symbols aggressively and performs any list operations it can 
reason about beforehand.


```java
!flat Point(x,y);
!flat Array2D(0,1);
!flat Field(Point start, Array2D end);

adder(Point a, Point b) = {
    x = a.x+b.x;
    y = a.y+b.y;
    return x,y;
}

Point p1 = 1,2;
Point p2 = p1;
Point p3 = adder(p1, p2);
print(p3.x);
print(p3);

Field f = p1,adder(p1,p2);
print(f.start);
print(f.start.y);
print(f.end.1);
```
