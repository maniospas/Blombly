# Blombly 

:computer: Expressive<br/> 
:duck: Dynamic<br/> 
:rocket: Parallel<br/> 
:goggles: Safe<br/> 

[Learn more...](https://maniospas.github.io/Blombly/) 

# Very quick start

Find the latest release [here](https://github.com/maniospas/Blombly/releases/tag/latest).
<br>Current build targets: *Windows x64*

Unzip the folder in a directory and create a file `main.bb` with the following contents:

```cpp
#include "libs/fmt"
enable fmt;  // pretty read and print

name = read("What's your name?");
print("Hello ", name, "!");
```

Run `blombly.exe main.bb`, where the executable and main files can be any path, and check that everything is working properly. 
Do not move the executable without all the `libs/` and dynamically linked libraries that are packaged with it.

# Build from source 

Clone this repository and install gcc in your system. Then, follow the steps below, which include installing the `vcpkg` dependency manager:

```bash
# install vcpkg
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # windows bootstraping (./bootstrap-vcpkg.sh for linux)
./vcpkg install zlib civetweb
cd ..

# build instructions
mkdir build
cmake -B ./build
cmake --build ./build --config Release  # rerun this after making changes
```

# Credits 

Author: Emmanouil (Manios) Krasanakis<br/> 
Contact: maniospas@hotmail.com<br/> 
License: Apache 2.0
