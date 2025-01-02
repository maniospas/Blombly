# <img src="docs/blombly.png" alt="Logo" width="32"> Blombly 


A simple programming language for creating reusable dynamic interfaces.

:triangular_flag_on_post: Reusable<br>
:rocket: Expressive<br>
:duck: Dynamic<br>
:bulb: Simple

## [Documentation](https://blombly.readthedocs.io/en/latest/)

## Install & run

Find the latest release [here](https://github.com/maniospas/Blombly/releases/latest).
<br>Current build targets: *Windows x64*, *Linux x64*
<br>Current integration testing targets: *Linux x64*

Unzip the release in a directory and create a file `main.bb` (or any name but with the same extension). Add the following contents:

```java
name = read("What's your name?");
print("Hello !{name}.");
```

Run `./blombly main.bb`, where the executable and main files can be any path, and check that everything is working properly. 
Do not move the executable without the packaged `libs/` directory and (in Windows) accompanying .dll libraries.

## A small example

```java
// main.bb (this is a line comment, strings used for multi-line comments)

"Below we define a code block (it does not run yet).
It is made immutabe with `final` for visibility from 
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
        return "({x}, {y})";
    }

    add(other) => new { // `=> ..` is `= {return ...}`
        Point: // inlines the code block
        // get values from the definition's immediate closure (this..)
        x = this..x+other.x; 
        y = this..y+other.y;
    }
}

a = new {Point:x=1;y=2}
b = new {Point:x=3;y=4}

// we defined `add`and `str`, which overload operations
c = a+b; 
print("{a} + {b} = {c}"); 
```

```text
> ./blombly main.bb
(1, 2) + (3, 4) = (4, 6) 
```

## Build from source 

Clone this repository and install gcc in your system. Then, follow the steps below, which include installing the vcpkg dependency manager:

```
# install vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # windows bootstraping (./bootstrap-vcpkg.sh for linux)
./vcpkg install zlib civetweb curl[core] --recurse 
cd ..

# install asmjit
git clone https://github.com/asmjit/asmjit.git

# build instructions
mkdir build
cmake -B ./build
cmake --build ./build --config Release  # rerun this after making changes
```

## Credits 

Author: Emmanouil (Manios) Krasanakis<br/> 
Contact: maniospas@hotmail.com<br/> 
License: Apache 2.0
