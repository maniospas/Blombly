# <img src="docs/blombly.png" alt="Logo" width="32"> Blombly 


A simple programming language for creating reusable dynamic interfaces.

:recycle: Reusable<br>
:rocket: Expressive<br>
:duck: Dynamic<br>
:bulb: Simple

## [Documentation](https://blombly.readthedocs.io/en/latest/)

## Install & run

Find the latest release [here](https://github.com/maniospas/Blombly/releases/latest).
<br>Build targets: *Windows x64*, *Linux x64*
<br>Integration testing: *Linux x64*

Unzip the release in a directory and create a file `main.bb`. Use any name but with the same extension. Add the following contents:

```java
// main.bb
name = read("What's your name?");
print("Hello !{name}."); // string interpolation with !{...}
```

Run `./blombly main.bb`, where the executable and main files can be any path, and check that everything is working properly. 
Do not move the executable without the packaged `libs/` directory and (in Windows) accompanying *.dll* libraries.

## A small example

```java
// main.bb (this is a line comment, strings used for multi-line comments)

"Below we define a code block (it does not run yet).
It is made immutable with `final` for visibility from 
function calls. Blocks can be called as functions or 
inlined, that is, dynamically pasted.";

final Point = {
    "We are planning to inline this block in structs 
    being created. That is, we will treat it as a 
    class that does not have reflection.";

    str() = {
        // struct field access and f-string
        x = this.x;
        y = this.y;
        return "(!{x}, !{y})";
    }

    add(other) => new { // `=> ..` is `= {return ...}`
        Point: // inlines the code block
        // get values from the definition's immediate closure (this..)
        x = this..x+other.x; 
        y = this..y+other.y;
    }
}

// structs keep creation vars, inline the code block with `:`
a = new {Point:x=1;y=2}
b = new {Point:x=3;y=4}

// we defined `add`and `str`, which overload operations
c = a+b; 
print("!{a} + !{b} = !{c}"); 
```

```text
> ./blombly main.bb
(1, 2) + (3, 4) = (4, 6) 
```

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
.\vcpkg.exe install sdl2 sdl2-image sdl2-ttf sqlite3 civetweb openssl zlib curl[core,ssl,ssh] --recurse
cd ..
```

Build the target. Change the number of processors to further speed up compilation; set it to at most to one less than the number of system cores.

```
cmake -B .\build
cmake --build .\build --config Release  --parallel 7
```

This will create `blombly.exe` and a bunch of *dll*s needed for its execution.


⚠️ I am not good enough with
cmake to force proper g++/mingw compilation and linking in both dependencies and the main compilation (I would appreciate some help there). So, in Windows with MSVC as the default compiler you will get an implementation with slower dynamic dispatch during execution. This mostly matters if you try
to do intensive numeric computations without vectors - which you really shouldn't.

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
./vcpkg install sqlite3 civetweb openssl zlib curl[core,ssl,ssh] --recurse
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
- [ ] JIT (progress: some preparation of future gcc compilation, not a priority to focus on engine optimizations)
- [X] Improve list semi-type semantics
- [ ] Simplify map creation
- [ ] CUDA vectors
- [x] Graphics: SDL (progress: implemented but unstable)
- [x] Graphics: Multiple windows, perhaps state-based management 
- [ ] Sound: SDL
- [X] Nameless gather (similar to Python's list comphrehension)
- [x] Clarified error handling (progress: errors as values, `fail` on side-effect failure, `do`)
- [ ] **Minimum size structs** (design progress: going to remove use-once variables which will remove all _bb intermediates)



## Credits 

Author: Emmanouil (Manios) Krasanakis<br/> 
Contact: maniospas@hotmail.com<br/> 
License: Apache 2.0