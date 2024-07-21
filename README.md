# Blombly

*A code block assembly language.* 

:computer: Fast onboarding<br/>
:duck: Duck typing<br/>
:rocket: Parallel<br/>
:goggles: Memory safety<br/>

## Material

[**Language guide**](docs/index.html)<br>

# Downloads

[Windows](../blombly.exe)

Build from source: Download this repository and install gcc in your system.
Run the following command from the directory of the `blombly.cpp` file:

```gcc
g++ -std=c++20 -pthread -I./include src/*.cpp src/data/*.cpp src/interpreter/*.cpp blombly.cpp -o blombly -O2
```

*O2 optimizations are necessary.*


## Credits

Author: Emmanouil (Manios) Krasanakis<br/>
Contact: maniospas@hotmail.com<br/>
License: Apache 2.0