# Keywords

## Syntax

| Keyword/Operator | Description                                                                                     |
|------------------|-------------------------------------------------------------------------------------------------|
| `!include`       | Imports code or files into the current scope.                                                   |
| `!access`        | Grants access to specific resources or properties.                                              |
| `!modify`        | Allows modification of a resource or property.                                                  |
| `!macro`         | Declares a macro that affects all subsequent code.                                              |
| `!local`         | Declares a macro that stays in effect until end of file (or until end of include brackets).     |
| `!gather`        | Initializes a value and defines an aggregation operation for it.                                |
| `yield`          | Aggregates a value to the anonymous value; only usable within `gather`.                         |
| `final`          | Marks a value or variable as immutable; cannot be reassigned.                                   |
| `default`        | Specifies fallback values.                                                                      |
| `:`              | Performs inline execution of its preceding code block/function.                                 |
| `new`            | Creates a new instance of a struct. Can return from it to just treat it as a new scope.         |
| `.`              | Accesses members (properties or methods) of an object. Consecutive occurrences grab values from creation closure,|
| `this`           | Refers to the current instance of a class or scope.                                             |
| `if`             | Introduces a conditional branch, executing code based on a condition.                           |
| `while`          | Executes a block of code repeatedly while a condition is true.                                  |
| `do`             | Intercepts returns and unhandled errors.                                                        |
| `catch`          | Handles errors like an `if` statement. There is no other struct reflection mechanism.           |
| `fail`           | Explicitly triggers an error.                                                                   |
| `return`         | Exits a function or block, optionally providing a value.                                        |
| `=>`             | Represents a lambda or function shorthand, or maps an input to an output.                       |
| `defer`          | Schedules a block of code to run at the end of the current scope (e.g., cleanup actions).       |
| `in`             | Iterates through an iterable be applying `iter` to it first.                                    |
| `clear`          | Empties or resets the contents of a collection or resource.                                     |
| `move`           | Transfers ownership of data while clearing the original one - applicable only when `clear` applies. |

## Operations

| Type/Operation    | Description                                                                                 |
|-------------------|---------------------------------------------------------------------------------------------|
| `int`             | Represents integer numbers.                                                                |
| `float`           | Represents decimal or floating-point numbers.                                              |
| `bool`            | Represents boolean values: `true` or `false`.                                              |
| `str`             | Represents sequences of characters (strings).                                              |
| `list`            | Represents an ordered collection of elements.                                              |
| `map`             | Represents a collection of key-value pairs.                                                |
| `set`             | Represents a collection of unique elements.                                                |
| `void`            | Represents the absence of a return value for a function or method.                         |
| `iter`            | Iterates over elements in a collection or resource.                                        |
| `range`           | Iterates through a specified range of values.                                              |
| `+`, `-`, `*`, `/`| Basic arithmetic operations: addition, subtraction, multiplication, and division.          |
| `%`               | Modulo operation, returns the remainder of a division.                                     |
| `^`               | Exponentiation operation.                                                                  |
| `==`, `!=`        | Equality and inequality comparison operators.                                              |
| `<`, `>`, `<=`, `>=` | Relational operators for comparing values.                                              |
| `and`, `or`, `not`| Logical operators for boolean logic.                                                       |
| `+=`, `-=`, `*=`, `/=` | Shorthand for self-assignment with arithmetic operations.                             |
| `[]`              | Indexing or slicing for lists, strings, or other collections.                              |
| `{}`              | Used to define code blocks.                                                                |
| `()`              | Used to group expressions or call functions and methods.                                   |
| `->`              | Indicates a return type in function signatures or represents a pointer in some contexts.   |
| `::`              | Separates positional arguments with preampled code execution during function calls.        |
| `|`               | Applying a single-argument function at the right to the argument at the left.              |
| `=`               | Assigns a value to a variable.                                                             |
| `as`              | Assigns a value to a variable while returning a bool on whether this was error-free.       |
