# Iterables

Here we cover builtin types that represent collections of elements.
These are `list`, `vector`, `map`, and the complementary `iter`
that helps loops iterate through elements. 


## Lists

Declare lists by separating values with a comma and access and set
its elements with square brackets (`[...]`) where element indexes
start from zero. Here is an example that also demonstrates usage of
the `len` builtin to obtain the number of list elements:

```java
// main.bb
A = 1,2,3;
A[0] = 4;
print(A);
print(len(A));
```

```bash
> blombly main.bb
[4, 2, 3]
3
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

The above examples do not modify list size while running. 
But Blombly also offers operators that do so, and in fact let you treat lists 
like stacks or queues too. In particular, use the `next` and `pop` operators 
to extract and remove the first and last list elements. These yield a missing value
if the list is empty, so the `as` operator in a loop can
extract list elements until either a missing one is found or the whole
list is consumed:

```bash
// main.bb
A = 1,2,3;
while(a as next(A)) 
    print(a);
```

```bash
> blombly main.bb
1
2
3
```

Append list elements using the `push` operator.  This 

## Iterators

Given any data structure, such as a list, that accepts an element getter and length methods,
you can create an iterator to travers through its elements wihtout modifying the structure. 
Obtain an iterator with the `iter` operation as in the example below. Iterators admit only the 
`next` operator and cannot modify data, expose random access, or restart.

```java
// main.bb
A = 1,2,3;
it = iter(A);
while(i as next(it)) 
    print(i);
```


## Vectors

*This segment is unders construction.*

## Maps

*This segment is unders construction.*