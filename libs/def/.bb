final def::INFO as {
    name    = "def";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.1";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces class and function definition semantics.
    \n All declarations provided by this library create
    \n final values.
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
    \n ----------
    \n Defines a class through its constructor. The 
    \n constructor is final and callable. Here is an
    \n example:
    \n 
    \n |   def::class Dog(name,breed) \{
    \n |       // need to declare fields with `use`
    \n |       // prefer semi-typing to sanitize inputs
    \n |       def::use name  | str;
    \n |       def::use breed | str;
    \n |   \}
    \n |   rex = Dog(\"rex\", \"german shepherd\");
    \n 
    \n def::struct
    \n -----------
    \n Defines and instantiates a final struct with new.
    \n Use this like so:
    \n 
    \n |   def::struct rex \{
    \n |       name = \"rex\";
    \n |   \}
    \n
    ";
} 


#include "libs/def/fn"
#include "libs/def/class"

// enables the 
#macro {def::simplify;} as {
    #local {fn} as {def::fn}
    #local {struct} as {def::struct}
    #local {abstract} as {def::abstract}
    #local {use} as {def::use}
    #local {class} as {def::class}
}
