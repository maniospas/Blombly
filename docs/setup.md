# Setup

## Hello world!

Let's make a first program to demonstrate the compilation and interpretation process. Create the following example file, where the `.bb` extension is associated with Blombly source code. 
Use a Java syntax highlighter (but not a syntax checker), as the two languages share many keywords.


```java
// main.bb
print("Hello world!");
```

To run this file, download Blombly's latest [release](https://github.com/maniospas/Blombly/releases/latest). Extract that into a folder
and add the latter to your filepath to let your operating system know where `blombly.exe` is located. Alternatively,
use the full path to the executable everywhere. Instructions to build the library from source are in the
[GitHub](https://github.com/maniospas/Blombly) page.


Once you set things up, run the following console command. 
If a message starting with `( ERROR )` appears, everything runs properly but there was some syntax or logic issue.
For example, brackets may have closed with a semicolon, or there could be some other type of infraction. More on errors below.

```bash
> blombly main.bb
Hello world!
```



## VM code

Compilation converts code to the intermediate representation of BlomlyVM. 
This representation is stored in files with the `.bbvm` extension.
These are self-contained by packing all dependencies inside, 
and therefore can be shared with others to run directly. 
The following file is generated when you run `main.bb` above.

```asm
% blombly.bbvm
BUILTIN _bb0 "Hello world!"
print # _bb0
```

```bash
> blombly main.bbvm
Hello world!
```

 To get a sense for internal commands, each one is a tuple of virtual machine instruction and its arguments
 are separated by spaces. The first argument is always a variable to assign to, where `#` indicates
 assignment to nothing. Temporary variables start with `_bb`, 
 and code blocks start by `BEGIN` or `BEGINFINAL` and end at `END`.

 Compilation optimizes the code for faster execution,
 for example by removing unused variables or code segments.
 This keeps representations small even if a lot of large dependent
 libraries are used.
 To convert the compilation outcome to semi-readable blombly code,
 run the Python script `bbreader.py` (this is intensionally written in a different language
 to also help with debugging during development).


## Errors

Finally, before jumping into actual coding, let us peek at errors that Blombly may create. There are two types:

- Syntax errors are identified by the compiler and make it halt.
- Logical errors occur at runtime and can be intercepted with `try` and separated from other data types with `catch`. This is the only usage of reflection in the whole language.

To see what a syntax error looks like, execute the following invalid code.
We get an error telling us that the + operation for string concatenation has no right-hand side. 
The compiler shows the exact position of the missing expression within the source code.

```java
// main.bb
print("Hello"+);  // CREATES AN ERROR
```

```text
> blombly main.bb
 (<ERROR>) Empty expression
   → print("Hello"+);  main.bb line 1
     ~~~~~~~~~~~~~~^
```

Look at a logical error by printing a variable that does not exist.
This is missing during interpretation and comprises a stack trace of compiled code. 
You will see full traces, regardless of which computations are internally delegated to threads.
Intercepting and handling errors like this is left for [later](advanced/signals.md).


```java
// main.bb
print(x);  // CREATES AN ERROR
```

## What's next?

Continue going through the language's documentation by clicking the navigation bar's "Next" button at the top.