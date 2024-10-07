# Setup

## Hello world!

Let's make a first program to demonstrate the compilation and interpretation process. Create the following example file, where the `.bb` extension is associated with Blombly source code. 
Use a Java syntax highlighter (but not a syntax checker), as the two languages share many keywords.


```java
// main.bb
print("Hello world!");
```

To run this file, download Blombly's latest [release](https://github.com/maniospas/Blombly/releases/latest). Extract the release to a folder
and add it to your filepath to let your operating system know where `blombly.exe` is located. If you don't want to add anything to your filepath,
use the full path to the executable everywhere. Instructions to build the library from source for your environment are provided in repository's
[GitHub](https://github.com/maniospas/Blombly) page.


Once you set up everything, run the following console command. 
If a message that starts with `( ERROR )` appears, the language was set up properly but there was some syntax or logic error.
For example, brackets may have closed with a semicolon, or there could be some other type of infraction. More on errors below.

```bash
> blombly main.bb
Hello world!
```



## VM code

Compilation converts code to the BlomlyVM intermediate representation language files, like the one below. Those files are associated with the .bbvm extension and look like assembly code. 
They are self-contained by packing all dependencies inside and can run directly; thus they can be shared directly for execution by others.

```asm
% blombly.bbvm
BUILTIN _bb0 "Hello world!"
print # _bb0
```

 To get a sense of internal commands temporary variables start with `_bb`. Compilation may have also optimized parts of the code for faster execution,
 for example by creating unused variables that let the virtual machine reduce memory allocations.


## Errors

Before jumping into actual coding, let us peek at errors that Blombly may create. Broadly, there are syntax errors, which are identified by the compiler and make it halt, and logical errors 
that occur at runtime and can be caught with appropriate try clauses to avoid program termination. 

To see what a syntax error looks like, let us try to execute the following invalid code.
We get an error telling us that the + operation for string concatenation has no right-hand side. The compiler shows the exact position of the missing expression within the source code.

```java
// main.bb
print("Hello"+);  // CREATES AN ERROR
```

```bash
> blombly main.bb
 (<ERROR>) Empty expression
   → std::print("Hello"+);  main.bb line 1
     ~~~~~~~~~~~~~~~~~~~^
```

Let us see what a logical error looks like by next printing a variable that does not exist.
This creates error during runtime that points to the stack trace of both compiled code and the original source. 
You will see full traces even when certain computations are internally delegated to threads.
For more complicated errors, the virtual machine shows the full stack trace leading to the failure.
This is retrieved even when code segments run in threads. Runtime errors can be caught to create self-healing code,
but this is a more advanced concept that we talk about [later](success-fail.md).


```java
// main.bb
print(x);  // CREATES AN ERROR
```
