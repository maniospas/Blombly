# <img src="docs/blombly.png" alt="Logo" width="32"> Blombly 


A simple programming language for creating reusable dynamic interfaces.

:recycle: Reusable<br>
:rocket: Expressive<br>
:duck: Dynamic<br>
:bulb: Simple

## [Documentation](https://blombly.readthedocs.io/en/latest/)

## Install & run

Build targets: *Windows x64*, *Linux x64*
<br>Integration testing: *Linux x64*

Find the latest release [here](https://github.com/maniospas/Blombly/releases/latest). You may also copy-paste one of the following installation commands.

<details><summary>Linux installation</summary>

Copy and past the following in your terminal.

```bash
mkdir -p /tmp/blombly_unpack && \
curl -L https://github.com/maniospas/Blombly/releases/download/v1.45.1/linux_release.tar.gz -o /tmp/blombly_unpack/linux_release.tar.gz && \
tar --no-same-owner --no-same-permissions -xzf /tmp/blombly_unpack/linux_release.tar.gz -C /tmp/blombly_unpack && \
sudo cp -r /tmp/blombly_unpack/libs /usr/local/bin/ && \
sudo cp /tmp/blombly_unpack/blombly /usr/local/bin/ && \
rm -rf /tmp/blombly_unpack
```

You can then run the executable from everywhere, for example per `blombly main.bb`.

</details><br>

Unzip the release in a directory and create a file `main.bb`. Use any name but with the same extension. Add the following contents:

```java
// main.bb
name = read("What's your name?");
print("Hello !{name}."); // string interpolation with !{...}
```

Run `./blombly main.bb`, where the executable and main files can be any path, and check that everything is working properly. 
Do not move the executable without the packaged `libs/` directory and (in Windows) accompanying *.dll* libraries.



## Terminal scripts

```bash
# prints if no semicolons
./blombly 'log(3)+1'
2.098612
```

```bash
# directly run code
./blombly 'n=10; fmt(x)=>x[".3f"]; print(fmt(2.5^n))'
9536.743
```

```bash
# the standard library is there too
./blombly 'bb.string.md5("this is a string")'
b37e16c620c055cf8207b999e3270e9b
```

## A small example

```java
// main.bb (this is a line comment,
// use strings for multi-line comments)

"Below we define a code block (it does not run yet).
It is made immutable with `final` for visibility from 
function calls. Blocks can be called as functions or 
inlined, that is, dynamically pasted.";

final Point = {
    "We are planning to inline this block in structs 
    being created. That is, we will treat it as a 
    class that does not have reflection.";

    str() = {
        // struct fields and f-string
        x = this.x;
        y = this.y;
        return "(!{x}, !{y})";
    }

    // `=> ..` is `= {return ...}`
    add(other) => new { 
        // `:` inline the block
        Point:
        // get values from closure 
        // (extra dot in this)
        x = this..x+other.x; 
        y = this..y+other.y;
    }

    norm() = {
        // set defaults if values not found
        default p=2;
        default toabs=true;
        x = this.x;
        y = this.y;
        if(toabs and x<0) x = 0-x;
        if(toabs and y<0) y = 0-y;
        return (x^p+y^p)^(1/p);
    }
}

// structs keep creation vars, 
// `:` inlines the block
a = new {Point:x=1;y=2}
b = new {Point:x=3;y=4}

// `add`and `str` overloads
c = a+b; 
print("!{a} + !{b} = !{c}"); 
// inject values in functions
print(c.norm(p=1;toabs=true));
```

```text
> ./blombly main.bb
(1, 2) + (3, 4) = (4, 6) 
10
```

## Code in your language

Experimentally, you can try to code in your language. This feature is not complete. Furthermore, its current development is LLM-aided because I'm just one STEM person that tries to help with accessibility of a side-project. Currently, error messages and standard library traces remain in English, but this will change going forward.

```java
!include "bb://libs/locale/gr"  // coding in Greek

// maxval = int("Give an integer"|read);
// while(x in range(0,maxval)) if(x%2==0) print("!{x} is even");
μέγιστος = ακέραιος("Δώσε έναν ακέραιο"|διάβασε);
όσο(χ σε διάστημα(0,μέγιστος)) αν(χ%2==0) εκτύπωσε("!{χ} είναι άρτιος");
```

🌎🌍🌏 **Looking for localization contributors.** I am looking for contributions that verify and edit the existing LLM localization files, which in this repository can be found `libs/locale/`, as well as potentially provide support for more languages. Localization is just a set of macros that replace several established symbols, and as a general rule should have one word per command. If you use an LLM to aid you (see existing files), please indicate so. In general create a pull request and add yourself as a contributor in the corresponding file. Reminder that contributions to this repository are distributed under the Apache 2.0 license.


## Build from source 

Follow the steps below, which include installing the vcpkg dependency manager.
Similar processes should work for other platforms too, but I have not tested them - I am actively looking for contributions on this.

<details>
<summary>Windows</summary>

Get vcpkg and use it to install dependencies. 

``` 
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg.exe install sdl2 sdl2-image sdl2-ttf sqlite3 civetweb openssl zlib curl[core,ssl,ssh] xxhash --recurse
cd ..
```

Build the target. Change the number of processors to further speed up compilation; set it to at most to one less than the number of system cores.

```
cmake -B .\build
cmake --build .\build --config Release  --parallel 7
```

This will create `blombly.exe` and a bunch of *dll*s needed for its execution.


⚠️ I am not good enough with
cmake to force proper g++/mingw compilation and linking in both dependencies and the main compilation. 
So, in Windows with MSVC as the default compiler you will get an implementation with slower dynamic dispatch during execution.
This mostly matters if you try to do intensive numeric computations without vectors - which you really shouldn't.

</details>

<details>
<summary>Linux</summary>

First install SDL2 separately, because the linux vcpkg installation is not working properly for me.

```
sudo apt-get install libsdl2-dev
sudo apt-get install libsdl2-image-dev
sudo apt-get install libsdl2-ttf-dev
```

Get vcpkg and use it to install the rest of the dependencies. 

```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg install sqlite3 civetweb openssl zlib curl[core,ssl,ssh] xxhash --recurse
cd ..
```

Build the target. Change the number of processors to further speed up compilation; set it to at most to one less than the number of system cores.

```
cmake -B ./build
cmake --build ./build --config Release  --parallel 7
```

</details>

## Batteries included

Development status of planned features:

- [x] Execution and creation closures
- [x] Memory management (refcounter, raii, defer, clear, move)
- [x] Automatic thread scheduling
- [x] Resource access and modification safety
- [x] Explicit filesystem name resolution
- [ ] Virtual filesystem (progress: works for files, not databases for now)
- [ ] Fuzz tests (depends on virtual filesystem)
- [x] Permission management when running bbvm files, build system
- [ ] Program management (e.g., permission summaries, UI for summaries)
- [x] Database: sqlite
- [x] Comptime
- [x] Compile optimizations
- [x] Vectors for scientific computations
- [ ] Matrices/tensors for scientific computations (may use eig)
- [x] REST server
- [x] Test suite (implemented through the standard library)
- [x] Macros
- [ ] Web sockets
- [x] HTTP, HTTPS clients
- [x] FTP, SFTP, FTPS clients
- [ ] FTP server (may be skipped)
- [ ] SSH
- [ ] JIT (progress: some preparation for future gcc compilation, not a priority to focus on engine optimizations)
- [X] Improve list currying semantics
- [X] Simplify map creation
- [ ] CUDA vectors
- [x] Graphics: SDL (progress: implemented but unstable)
- [x] Graphics: Multiple windows, perhaps state-based management 
- [ ] Sound: SDL
- [X] Nameless gather (similar to Python's list comphrehension)
- [x] Clarified error handling (progress: errors as values, `fail` on side-effect failure, `do`)
- [X] Minimum size structs (removed _bb intermediates)
- [ ] Fast error handling
- [ ] Re-enable threading for the final computational model
- [X] Elegant include system (automatic removal of duplicate code that allows re-includes, include bbvm files, proper include tracing)
- [ ] Process attachment and external code execution (enables CLI/debugging)


## Credits 

Author: Emmanouil (Manios) Krasanakis<br/> 
Contact: maniospas@hotmail.com<br/> 
License: Apache 2.0