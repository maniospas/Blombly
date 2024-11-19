final def::INFO as {
    name    = "final";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.1";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces class and function definition semantics.
    \n All declarations provided by this library create
    \n final values, thus the name as a mnemonic.
    \n 
    \n final::def
    \n ----------
    \n Defines a function. This is a final code block with a 
    \n given name and arguments. Positional arguments
    \n (in fact, a runnable
    \n default code block) are also supported like below.
    \n Additional arguments can also be provided.
    \n
    \n |   final::def adder(x, y|bias=0) \{return x+y+bias\}
    \n
    \n final::class
    \n ------------
    \n Defines a class through its constructor. The 
    \n constructor is final and callable. Here is an
    \n example:
    \n 
    \n |   final::class Dog(name,breed) \{
    \n |       // need to declare fields with `use`
    \n |       // prefer semi-typing to sanitize inputs
    \n |       def::use name  | str;
    \n |       def::use breed | str;
    \n |   \}
    \n |   rex = Dog(\"rex\", \"german shepherd\");
    \n 
    \n final::struct
    \n -------------
    \n Defines and instantiates a final struct with new.
    \n Use this like so:
    \n 
    \n |   final::struct rex \{
    \n |       name = \"rex\";
    \n |   \}
    \n
    ";
} 


#include "libs/final/fn"
#include "libs/final/class"
