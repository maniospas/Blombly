# Iterables

Here we cover builtin types that represent collections of elements.
These are `list`, `vector`, `map`, and the complementary `iter`
that helps loops iterate through elements. 


## Lists

Declare lists by separating values with a comma and access and set
its elements with square brackets (`[...]`) where element indexes
start from zero. Here is an example:

```java
// main.bb
A = 1,2,3;
A[0] = 4;
print(A);
```

```bash
> blombly main.bb
[4, 2, 3]
```

You may initialize lists based on their size and gradually fill
in elements. Elements not set yet have missing values, which means that 
there will be an error if you try to retrieve them unless the `as` assignment is used:

```java
// main.bb
A = list(5); // list of 5 elements
A[0] = 0;
A[3] = 0;
A[4] = 0;
if(x as A[1]) print(x) else print("Element at position 1 not set");
print(A);
```

```bash
> blombly main.bb
Element at position 1 not set
[0,  ,  , 0, 0]
```

## Iterators

*This section is under construction.*

