# Code blocks

## Inlining

The main logic compartmentalization mechanism in Blombly are code blocks; these are flexible coding segments that can be treated as methods, used to define various control flows, or called inline. Code blocks are declared by enclosing them in brackets and assigning them to a variable, like below. Notice that there is no trailing semicolon, and the compiler will create an error if you do try to add one, under the principle of "one solution". The declaration can also be final to make sure that the same variable always points to the particular block in the current scope, and that method calls starting from the scope have access to it.

The declaration itself only sets a variable. But you can effectively "paste" a block's internal code to the current position by using the block's name followed by double dots ()`:`). This is called inline execution and demonstrated below. Inlining has full access to variables in the current scope (both for usage and modification). The main usage pattern is to enrich your current code with a snippet defined elsewhere, which may also change dynamically.

```java
// main.bb
test = { 
    print("Hello world"); 
} 
test:
```

## Specifications

We tackle usage of code blocks as methods in the next section. For now, let us see how to declare and retrieve some specification for them. In the simplest case, Blombly offers the dot . notation, like the language's structs described in section 2.5 to read and write specification data. The latter differ from struct fields in that they declare immutable properties, like documentation and source code versions. When assigning specifications without evoking a preprocessor directive, use the final keyword to signify their immutable nature. For logic safety, an error will occur otherwise. Here is an example usage:

```java
// main.bb
final test = { // ensure that this does not change in current scope 
    print("Hello world! I am a running code block."); 
} 
final test.version = 1.0; // setting metadata is always final final test.license = "CC0";
print("Running test version "+str(test.version) +" under "+test.license+" license...");
test: // block code directly runs here
```

A shorthand for declaring specifications consists of defining them at the beginning of blocks using the #spec directive for the preprocessor. This declaration cannot use any of the block's variables, but uses the variables from the block's declaring scope, as it is interpreted equivalently to above. That is, the metadata sets are performed in the declaring scope just after the block declaration finishes. Importantly, the same notation lets us treat source code files as blocks that also have metadata.

As a coding standard, usage of the following metadata fields is encouraged:

```java
// main.bb
#spec doc = "This is a documented program."; 
#spec author = "Your Name (email@example.com)"; 
#spec version = 1.0;

final example = { 
    #spec doc = "This is a documented code block";
    print("Hello world! I am a documented method."); 
}
```


## Method calling

There are two mechanisms for creating callable code blocks (aka methods) in Blombly. The first relies on directly calling defined blocks by passing variables as keyword arguments. The second modifies block definitions so that they accept positional arguments, or a mix of positional and keyword arguments.

Calls to blocks other than builtin methods (or those overloading builtin methods in structs) run in parallel. Details of this orchestration are purposefully hidden so that you can write code as if all calls were sequential instead. In addition to their arguments, called blocks can access final values from the scope of their declaration.

Code blocks defined in Blombly can be called like methods by injecting values as keyword arguments separated by semicolons. Actually, the arguments constitute a bracketless code block that runs in an interim new context whose assignments are transferred to the code call. When not transforming data, it is recommended to stick to keywords for better readability and maintainability per the principles of forward-oriented programming that Blombly follows. For example, keyword arguments create a sense of the functional role of each argument for those reading code.

The return keyword ends block execution with the corresponding returning value. That value is intercepted by the block call in the scope of this section. However, the return signal may also be intercepted by the new mechanism of the next section and the try mechanism of section 3.2. Beware that inlining such blocks transfers their return statements to parent blocks too. There is only one returned value, but as always you can use commas between several values to create lists of returned values.

The example below defines a simple block and calls it with specific values for the inputs it needs to execute. The particular declaration is made final for logic safety, but this does not need to always be the case.



```java 
// main.bb
final add = { return x + y; }
result = add(x=1;y=2); 
print(result);
```

It was already mentioned that the space reserved for keyword arguments in block calls can in general be any other code block (without the brackets, as it requires fewer symbols to write and the range is obvious from the call's parentheses). This means that you can also inline other blocks that serve as configuration for multiple calls. This pattern is demonstrated below, alongside usage of the default keyword to set variables if not already declared.

```java
// main.bb
final add = { 
    default scale = 1; 
    return ((x/scale) + (y/scale))*scale; 
} 
config = {scale=2} 
result = add(config:x=1;y=2); 
print(result);
```

## Positional arguments

Blombly also supports calling blocks using comma-separated positional arguments. This is a common programming language pattern, and there are two ways to enable it; both tackle how to retrieve positional values from within called blocks. The lower-level option is to make use of the args variable generated by the compiler, which expressly holds positional arguments as a list. For code safety, this is considered a language keyword and you are not allowed to assign to it. You can then access list elements or use next to obtain their values (reminder that this pops the first list element, contrary to pop that pops the last). An example of this coding pattern is shown below.

```java
// main.bb
final add = { 
    x = next(args); 
    y = next(args); 
    return x + y; 
}
result = add(1, 2); 
print(result);
```

A second and higher-level way of parsing positional arguments is to add their names inside a parenthesis next to the block's name, separated by commas. This is a shorthand and still lets the block be assigned to other variables per normal. Blocks declared this way can be inlined directly within other called blocks (because they must use their scope's args). The following example is equivalent to the previous one. Finally, the implicit next removes the first entries from the arguments; you can still access the rest if more are provided in the call.

```java
// main.bb
final add(x, y) = { // shorthand for front-popping these values with next 
    return x + y; 
} 
test = add; // can still transfer the definition this way
result = test(1, 2);
print(result);
```

## Both types of arguments

Blombly allows mixing of positional and "keyword" arguments in a single call with a pattern that reads similarly to conditional probabilities name(args | kwargs) where args are the positional arguments and kwargs the bracketless code block that is used to generate the keyword arguments. Positional arguments are generated first from the calling scope, and can therefore modify the args if needed.

In the following example, the block add is defined with two parameters, x and y, and two default parameters, wx and wy. When calling add, we pass the numbers 1 and 2 as positional arguments, which are internally matched to x and y. Knowing the details about default parameters is not necessary at the point where the decision to use the block is made.

```java
// main.bb
final add(x, y) = { 
    default wx = 1; 
    default wy = 1; 
    return xwx + ywy;
}
result = add(1,2 | wx=1;wy=2); 
print(result);
```