final def::INFO as {
    name    = "def";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces class and function definition semantics
    \n that make code look similar to other languages.
    \n 
    \n def::fn
    \n -------
    \n Defines a final code block with a given name and
    \n arguments. Positional arguments (in fact, a runnable
    \n default code block) are also supported like below.
    \n Additional arguments can also be provided.
    \n
    \n |   def::fn adder(x, y|bias=0) \{return x+y+bias\}
    \n 
    \n def::class
    \n Defines a class through its constructor. The 
    \n constructor is final and callable. Here is an
    \n example:
    \n 
    \n |   def::class Dog(name) \{def::uses name;\}
    ";
} 


#include "libs/def/fn"
#include "libs/def/class"

// enables the 
#macro {def::simplify;} as {
    #macro {fn} as {def::fn}
    #macro {module} as {def::module}
    #macro {abstract} as {def::abstract}
    #macro {uses} as {def::uses}
    #macro {class} as {def::class}
}