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

```text
> ./blombly main.bb
Hello world!
```



## VM code

Compilation converts code to the intermediate representation of BlomlyVM. 
This representation is stored in files with the `.bbvm` extension.
These are self-contained by packing all dependencies inside, 
and therefore can be shared with others to run directly. 
The following `main.bbvm` file is generated when you run the above snippet.

```java
%("Hello world!") //main.bb line 1
BUILTIN _bb162 "Hello world!"
%print("Hello world!") //main.bb line 1
print # _bb162
```

```text
> ./blombly main.bbvm
Hello world!
```

To get a sense for internal commands, lines starting with `%` contain
debugging info and can be ignored. The rest of the commands are space-separated 
tuples of virtual machine instructions. The first element is the command name and the
second a variable to assign to, where `#` indicates
assignment to nothing. Temporary variables have the `_bb` prefix,
and code blocks start by `BEGIN` or `BEGINFINAL` and end at `END`.

Compilation optimizes the code for faster execution,
for example by removing unused variables or code segments.
For example, notice that above there are no needless instructions
from the standard library `libs/.bb`, despite this being
imported in every program.

Do not generate debugging symbols during compilation with the `--strip` or `-s` option.
This lets compilation and optimization finish faster while producing a much smaller bbvm file - arround 
half the size. For example, below is the same compilation outcome
with stripped away debug info. In this case, any errors will contain virtual machine instructions
instead of a source code stack traces.

```text
> ./blombly main.bb --strip
Hello world!
> cat main.bbvm
BUILTIN _bb162 "Hello world!"
print # _bb162
```


## Errors

Before jumping into actual coding, let us peek at errors that Blombly may create. There are two types:

- Syntax errors are identified by the compiler and make it halt.
- Logical errors occur at runtime. They are intercepted with `try` and identified with `catch`.

To see what a syntax error looks like, execute the following invalid code.
We get an error telling us that the + operation for string concatenation has no right-hand side. 
The compiler shows the exact position of the missing expression within the source code.

```java
// main.bb
print("Hello"+);  // CREATES AN ERROR
```

```text
> ./blombly main.bb
( ERROR ) Empty expression
   → print("Hello"+);                                     main.bb line 1
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

```text
> ./blombly main.bb
( ERROR ) Missing value: x
   → print(x)                                            main.bb line 2
```

*Logical errors do not point to the exact position in the code but only at the
expression being parsed. Follow the stack trace to the corresponding files for 
the full source code.*
