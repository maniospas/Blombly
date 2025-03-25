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

Contains helper methods for list and map manipulation using the semi-type notation. 
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