# Setup

## Get ready

Download Blombly's latest [release](https://github.com/maniospas/Blombly/releases/latest). Extract that into a folder
and add the latter to your filepath to let your operating system know where the main executable is located. Alternatively,
use the full path to the executable everywhere. Instructions to build the library from source are in the
[GitHub](https://github.com/maniospas/Blombly) page.
When writting in the language, use a Java keyword highlighter (but not syntax checker).

## Hello world!

Create the following example file, where the *.bb* extension is associated with Blombly source code.
Then run the language's interpreter followed by the file name.
If a message starting with `( ERROR )` appears, everything runs properly but there was some syntax or logic issue.

```java
// main.bb
print("Hello world!");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>



## VM code

The interpreter first compiles code to the intermediate representation of the
**BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM). 
Virtual machine instructions are stored in files with the *.bbvm* extension,
which are self-contained by packing all dependencies inside.
Share them with others to run directly. 
The following *main.bbvm* file is generated from the previous snippet.

```java
%("Hello world!") //main.bb line 1
BUILTIN _bb162 "Hello world!"
%print("Hello world!") //main.bb line 1
print # _bb162
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bbvm
Hello world!
</pre>

To get a sense for internal commands, lines starting with `%` contain
debugging info and can be ignored. The rest of the commands are space-separated 
tuples of virtual machine instructions. The first element is the command name and the
second a variable to assign to, where `#` indicates
assignment to nothing. Temporary variables have the `_bb` prefix,
and code blocks start at `BEGIN` or `BEGINFINAL` and end at `END`.

## Arguments

Set the number of operating system threads that the virtual machine
is allowed to use with the option `--threads <num>` or `-t <num>`.
If you specify nothing, the maximum amount is used. 
Set to zero threads to compile without executing.

<br>

Compilation optimizes the code by removing many unused variables or segments.
For example, notice that above there are no needless instructions
from the standard library *libs/.bb*, despite the latter being
imported in every program. If your plan to produce bbvm files
to be used as libraries (to be optimized by programs using them),
retain everything with the `--library` or `-l` option. 
Below is an example that compiles a file without running it while switching
between applying and not applying optimizations. The `du` linux
utility is used to show file sizes. Notice the bloat comming from the standard
library when there is no optimization!


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --threads 0
> <span style="color: cyan;">du</span>  -sh main.bbvm
4.0K    main.bbvm
> <span style="color: cyan;">./blombly</span> main.bb  --threads 0 --library
> <span style="color: cyan;">du</span>  -sh main.bbvm
48K     main.bbvm
</pre>

Avoid generating debugging symbols with the `--strip` or `-s` option.
This speeds up compilation and optimization and produces a smaller bbvm file - around 
half the size. For example, below is a compilation outcome
with stripped away debug info. In this case, any errors contain virtual machine instructions
instead of a source code stack traces.

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --strip
Hello world!
> <span style="color: cyan;">cat</span> main.bbvm
BUILTIN _bb162 "Hello world!"
print # _bb162
</pre>


## Errors

Before jumping into actual coding, let us peek at errors that Blombly may create. There are two types. 
First, syntax errors make the compiler halt.
Second, logical errors occur at runtime, are intercepted with `try`, and handled with `catch`.
To see what a syntax error looks like, execute the following invalid code.
We get an error telling us that the + operation for string concatenation has no right-hand side. 
The compiler shows the exact position of the missing expression within the source code.

```java
// main.bb
print("Hello"+);  // CREATES AN ERROR
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
(<span style="color: red;"> ERROR </span>) Empty expression
   <span style="color: lightblue;">→</span>  print("Hello"+);                                     main.bb line 1
      <span style="color: red;">~~~~~~~~~~~~~~^</span>
</pre>


Look at a logical error by printing a variable that does not exist.
This is missing during interpretation and comprises a stack trace of compiled code. 
You will see full traces, regardless of which computations are internally delegated to threads.
Errors like this can be intercepted and handled.


```java
// main.bb
print(x);  // CREATES AN ERROR
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
(<span style="color: red;"> ERROR </span>) Missing value: x
   <span style="color: lightblue;">→</span>  print(x)                                            main.bb line 2
</pre>


*Logical errors do not point to the exact position in the code but only at the
expression being parsed. Follow the stack trace to the corresponding files for 
the full source code.*
