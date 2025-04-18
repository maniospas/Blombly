# Setup

## Download & install

Download the latest Blombly [release](https://github.com/maniospas/Blombly/releases/latest). Extract it into a folder
and add that to your system's PATH to make the executable accessible. 
Alternatively, use the full path to the executable. 
If you prefer building from source, follow the instructions on the
[GitHub](https://github.com/maniospas/Blombly) repository.
You may also copy-paste one of the following installation commands.

<details><summary>Linux installation</summary>

Copy and paste the following in your terminal. This downloads, unzips, and moves 
all necessary files to <i>/usr/local/bin/</i>.
Then run the executable without specifying a path, for example per <code>blombly main.bb</code>.

```bash
mkdir -p /tmp/blombly_unpack && \
curl -L https://github.com/maniospas/Blombly/releases/download/v1.45.1/linux_release.tar.gz -o /tmp/blombly_unpack/linux_release.tar.gz && \
tar --no-same-owner --no-same-permissions -xzf /tmp/blombly_unpack/linux_release.tar.gz -C /tmp/blombly_unpack && \
sudo cp -r /tmp/blombly_unpack/libs /usr/local/bin/ && \
sudo cp /tmp/blombly_unpack/blombly /usr/local/bin/ && \
rm -rf /tmp/blombly_unpack
```

</details>

For better development experience, use a Java keyword highlighter but not syntax checker. 
VSCode is an editor that works well with the compiler because it lets you ctrl+click on
error messages to navigate through the code. A preliminary language server is also available as a VSCode extension
in [this link](https://github.com/maniospas/Blombly/raw/refs/heads/main/blombly-lsp/blombly-lsp-0.0.1.vsix).
Install *python3* and then the extension with the following instruction:

```bash
code --install-extension blombly-lsp-0.0.1.vsix
```

## Hello world!

To check that everything works, create the following example file. The *.bb* extension is associated with Blombly source code.
Then run Blombly's interpreter followed by the file name.
If a message starting with `( ERROR )` appears, everything runs properly but there was a syntax or logic issue.
Syntax errors make the compiler halt, whereas logic errors occur at runtime. 
They also include preliminary checks about logical errors that would be guaranteed to be encountered at runtime,
such as using varialbles that are never set anywhere - usually indicative of typos.

```java
// main.bb
print("Hello world!");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Hello world!
</pre>


Syntax errors require code fixing, but logic errors can be intercepted 
during runtime by functions returns and `do` statements. Details on how to handle 
them are found [here](advanced/try.md). For now it suffices to know
that, if left unhandled, they  cascade through the call stack until they reach your main program and appear to you. 
Below is a logical error that arises when converting an invalid string to a float and trying to add to it;
a full stack trace is obtained from the first issue to the 
last affected computation. Stack traces reflect how expressions are understood by the parser, 
and may have a slightly different formatting than your code.


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



## Virtual machine

The interpreter compiles code to the intermediate representation of the
**BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM). This strives for
safety and user control over consumed resources. Its instructions are stored 
in files with the *.bbvm* extension; these are self-contained by packing all 
dependencies inside. Share them with others to run directly. 

<br>

Those files are normally compressed with zlib, but you can save them in human
readable form with the `--text` option. They are executed normally regardless
on whether they are compressed or or not.
Below are example contents of such a file, where lines starting with `%` contain
debugging info and can be ignored. The rest of the file contains space-separated 
tuples of virtual machine instructions. The first element is the command name and the
second a variable to assign to, where `#` indicates
assignment to nothing. Temporary variables have the `_bb` prefix,
and code blocks start at `BEGIN` or `BEGINFINAL` and end at `END`.


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --text
Hello world!
> <span style="color: cyan;">cat</span> main.bbvm
</pre>

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

!!! tip
    If you specify no file name, *main.bb* is assumed. This lets you
    deploy the blombly executable and the *libs/* directory
    alongside that main file and have your users double-click 
    on the executable to run your application.

The virtual machine can be controlled in terms of some basic functionality that
permeates its running behavior. Options presented here are immutable.
More advanced functionality, such as resource read and write permissions, are managed
programmatically from the main file with preprocessor instructions.

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
For example, notice that so far there have been no needless instructions
from the standard library *libs/.bb*, despite the latter being
imported in every program. Produce bbvm files
to be used as libraries (these can be included and optimized by programs using them),
retain everything with the `--library` or `-l` option. Accompany this
with `--norun` to prevent running library code and creating errors from unset
variables.
Below is an example that compiles a file without running it while switching
between applying and not applying optimizations. The `du` linux
utility shows file sizes, here set to bytes. 
Notice the bloat coming from the standard library when there is no optimization!


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --norun
> <span style="color: cyan;">du</span>  -b main.bbvm
92      main.bbvm
> <span style="color: cyan;">./blombly</span> main.bb  --norun --library
> <span style="color: cyan;">du</span>  -b main.bbvm
10866   main.bbvm
</pre>

<br>

**--strip**

Avoid generating debugging symbols with the `--strip` or `-s` option.
This speeds up compilation and optimization and produces a smaller bbvm file - around 
half the size. For example, below is a compilation outcome
with stripped away debug info. In this case, any errors point to virtual machine instructions
instead of a source code stack traces, so they may be a harder to debug.
If the *bbvm* file is compressed, you won't be able to meaningfully delve into
its code either.

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --norun --library --strip
> <span style="color: cyan;">du</span>  -b main.bbvm
5875    main.bbvm
</pre>

<br>


**--text**

Normally, compiled *bbvm* files are compressed using zlib; this typically reduces their
size to a fourth of what it would normally be. Use the `--text` option
if you want them to be saved as pure text containing intermediate representations. 
The virtual machine can read both compressed and text versions interchangeably.

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb  --strip --text
Hello world!
> <span style="color: cyan;">cat</span> main.bbvm
BUILTIN _bb162 "Hello world!"
print # _bb162
</pre>

<br>

**--depth**

One of Blombly's safety mechanisms is a maximum depth of control flow derailment.
For example, this limits recursive functions from breaking your code. 
Inline execution of code blocks
-including the conditions and bodies of control flow statements- may count
towards this number unless it is automatically inserted at compile time. 
The default limit is *42* (the Answer!). Set a different value
with the option `--depth <num>` or `-d <num>`, but it is better to 
try to keep things simple by not exceeding the default maximum depth,

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

## Terminal utility

Blombly comes alongside several standard library implementation. This means
that you can use it not only as a simple calculator but also as a terminal
utility that grants direct access to those implementations.
Just enclose code provided through the terminal in single quotes (`'`). For ease of use, if no semicolon is 
found in code provided through command line arguments, that code is enclosed in
a print statement. Here are some examples:

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> 'log(3)+1'
2.098612
> <span style="color: cyan;">./blombly</span> 'n=10; fmt(x)=>x[".3f"]; print(fmt(2.5^n))'
9536.743
> <span style="color: cyan;">./blombly</span> 'bb.string.md5("this is a string")'
b37e16c620c055cf8207b999e3270e9b
</pre>
