# Setup

## Get ready

Download Blombly's latest [release](https://github.com/maniospas/Blombly/releases/latest). Extract that into a folder
and add the latter to your filepath to let your operating system know where the main executable is located. 
Alternatively, use the full path to the executable everywhere. 
Instructions for building from source are in the
[GitHub](https://github.com/maniospas/Blombly) page.

## Highlighting

When writing in the language, use a Java keyword highlighter (but not syntax checker). For the time being, it is also recommended 
that you use VSCode as the editor of choice, because you can also ctrl+click on files mentioned in
error messages to jump to mentioned positions. There is an immature language server available as a VSCode extension.
This requires python3 to be available in your system. Install it by downloading it 
from [this link](https://github.com/maniospas/Blombly/raw/refs/heads/main/blombly-lsp/blombly-lsp-0.0.1.vsix)
and running the following instruction in your terminal:

```bash
code --install-extension blombly-lsp-0.0.1.vsix
```


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


There are two types of errors that you may encounter. 
First, syntax errors make the compiler halt. 
To see what they look like, execute the following invalid code.
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


Second, logical errors occur at runtime, are intercepted once functions return (or by `do`), 
and are handled with `catch` or `as`. Details on logical error
handling can be found [here](advanced/try.md). For now it suffices to know
that, if left unhandled, runtime errors cascade through the call stack until they reach your main program and appear to you. Compilation contains preliminary checks about common logical errors that would be guaranteed to be encountered at runtime, saving the hussle of detecting them much later. 
One such error is using variables that are never set anywhere.

<br>

Look at a logical error by converting an invalid string to a float and trying to add to it.
A full stack trace is obtained from the first issue to the 
last affected computation. Stack traces reflect how expressions are viewed by the parser, 
which may look slightly different formatting than your code.
Follow traces to the corresponding file and line for 
the full source code, for example by ctrl+clicking on it in the VSCode terminal.
Logical errors can be caught and handled without breaking execution.


```java
// main.bb
x = float("foo");  // CREATES AN ERROR
print(x+1);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
(<span style="color: red;"> ERROR </span>) Not implemented: float(code)
   <span style="color: lightblue;">→</span>  float("foo")                                        main.bb line 1
   <span style="color: lightblue;">→</span>  x+1                                                 main.bb line 1
</pre>



## Virtual Machine

The interpreter first compiles code to the intermediate representation of the
**BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM). This strives for
safety and user control over consumed resources with options shown below.
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

!!! info 
    In internal commands, lines starting with `%` contain
    debugging info and can be ignored. The rest of the commands are space-separated 
    tuples of virtual machine instructions. The first element is the command name and the
    second a variable to assign to, where `#` indicates
    assignment to nothing. Temporary variables have the `_bb` prefix,
    and code blocks start at `BEGIN` or `BEGINFINAL` and end at `END`.

<br>

**--threads**

Set the number of operating system threads that the virtual machine
is allowed to use with the option `--threads <num>` or `-t <num>`.
If you specify nothing, the maximum amount is used. 

<br>

**--norun**

Use the `--norun` option as a shorthand for setting the thread number to zero. 
In this case, only a **.bbvm* file is produced without being executed. 
Compilation may still run some tasks triggered by preprocessor instructions 
(such as code that runs in comptime).

<br>

**--library**

Compilation optimizes the code by removing many unused variables or segments.
For example, notice that above there are no needless instructions
from the standard library *libs/.bb*, despite the latter being
imported in every program. To produce bbvm files
to be used as libraries (to be optimized by programs using them 
*in future versions*),
retain everything with the `--library` or `-l` option. 
Below is an example that compiles a file without running it while switching
between applying and not applying optimizations. The `du` linux
utility shows file sizes in kilobytes. 
Notice the bloat coming from the standard library when there is no optimization!


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --norun
> <span style="color: cyan;">du</span>  -sh main.bbvm
4.0K    main.bbvm
> <span style="color: cyan;">./blombly</span> main.bb  --norun --library
> <span style="color: cyan;">du</span>  -sh main.bbvm
48K     main.bbvm
</pre>

<br>

**--strip**

Avoid generating debugging symbols with the `--strip` or `-s` option.
This speeds up compilation and optimization and produces a smaller bbvm file - around 
half the size. For example, below is a compilation outcome
with stripped away debug info. In this case, any errors point to virtual machine instructions
instead of a source code stack traces, so they may be a harder to debug.

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --strip
Hello world!
> <span style="color: cyan;">cat</span> main.bbvm
BUILTIN _bb162 "Hello world!"
print # _bb162
</pre>

<br>

**--depth**

One of Blombly's safety mechanisms is a limitation on stack depth.
For example, this limits recursive functions from breaking your code. The
default limit is *42* (the Answer!) but you can set a higher or lower value
with the option `--depth <num>` or `-d <num>`. Inline execution of code blocks
-including the conditions and bodies of control flow statements- may count
towards this number unless it is automatically inserted at compile time. 
Do try to keep things simple by not exceeding the default maximum depth.

<br>

**multiple main files**

Provide multiple source files to compile and run in their order of 
occurrence. Consider those files as steps of a build process that
executes programs in the same virtual machine one after the other.
Command line arguments apply to all of the programs, and [IO](basics/io.md) 
configuration described in the user guide persist between the steps. 
For example, resource permissions and the virtual file system are carried over, 
which is also convenient for creating permission files to enable resources for
the execution of *.bbvm* files in next steps (permissions are *not* embedded in compiled files).
Directly run short code snippets instead of files in some steps
by adding them as console arguments enclosed in single quotes. An example follows.

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb 'print("Hi from the terminal.");'
Hello world!
Hi from the terminal.
</pre>
