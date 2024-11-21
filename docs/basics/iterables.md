# Iterables

Here we cover builtin types that represent collections of elements.
These are `list`, `vector`, `map`, and the complementary `iter`
that helps loops iterate through elements. All the aforementioned
data types are called iterables because they can successfully be
typecasted into `iter` instances. Some [structs](structs.md) are
also iterables, but more on this later.


## Lists

Declare lists by separating values with a comma. Access and set
its elements with square brackets (`[...]`) where element indexes
start from zero. Here is an example that also demonstrates usage of
the `len` builtin to obtain the number of elements:

```java
// main.bb
A = 1,2,3;
A[0] = 4;
print(A);
print(len(A));  // or print(A|len) is equivalent
```

```bash
> blombly main.bb
[4, 2, 3]
3
```

You may initialize lists based on one element like below.
This notation in general converts data to lists (e.g., 
can be overloaded by structs) which means that
you may have more elements in the parenthesis without 
issue if you want to write verbose code.

For out-of-bounds indexes, an error will be created when 
trying to set to them, and a missing value is returned when trying
to access them. 
This way, they create errors unless the `as` assignment is 
used to check for validity if list size is unknown.
Since it is impossible to have missing values within lists,
returned missing values always correspond to missing
invalid positions.

```java
// main.bb
A = list(5);
print(A);
A[3] = 0; // CREATES AN ERROR
```

```bash
> blombly main.bb
[5]
( ERROR ) List index 3 out of range [0,1)
   → put # A _bb3 _bb2                           main.bbvm line 6
```

The above examples do not modify lists during execution.
This is instead achieved with the `next` and `pop` operators 
that extract and remove the first or last list elements respectively. 
They yield missing values if the list is empty. 
Thus, the `as` assignment in a loop can
extract list elements until either a missing one is found or the whole
list is consumed:

```java
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

Append elements at the end using the `push` operator,
like so:

```java
// main.bb
A = 1,2,3;
push(A, 4);
print(A);
```

```bash
> blombly main.bb
[1, 2, 3, 4]
```

You may concatenate lists by adding them like below:

```java
// main.bb
print((1,2,3)+(4,5));
```

```bash
> blombly main.bb
[1, 2, 3, 4, 5]
```

Finally, lists support sub-indexing by any iterable that contains integer identifiers.
Internally, they obtain an iterator from that iterable and traverse its values like below.
Value repetitions are allowed. If the iterable contains missing values (this
happens only for certain iterable structs), sub-indexing will stop at the first missing value. 

```java
A = 1,2,3,4,5;
print(A[1,3]);
```

```bash
> blombly main.bb
[2, 4] 
```

## Iterators

Given any data structure that accepts an element getter and length methods,
you can create an iterator to traverses through its elements witjout modifying it. 
Such data include lists and are called iterables.

Obtain an iterator from an iterable with the `iter` typecasting as in the example below. 
Iterators admit only the `next` operator, and always accept a convertion to `iter` that
returns themselves. This is mostly done for ease of code writting. 
One cannot modify data, perform random access, or restart traversal of the iterable's elements. 

```java
// main.bb
A = 1,2,3;
it = iter(A);
while(i as next(it)) 
    print(i);
```


You can also create range-based iterators  using one of the expressions
`range(end)`, `range(start, end)`, or `range(start, end, step)`,
where the range starts and ends at integer values, and the end is non-inclusive.
In the first version, the `start=0`, and in the first two 
the `step=1`. Here is an example:

```java
// main.bb
it = range(3);
while(i as next(it))
    print(i);
```


If all three arguments are used and any of them is a float number, the range will
output floats everywhere. Negative steps are also allowed.
Below is an example that demonstrates both of these concepts.

```java
// main.bb
it = range(2, 0, -0.5);
while(i as next(it))
    print(i);
```

```bash
> blombly main.bb
2.000000 
1.500000
1.000000
0.500000
```

## `in` iteration

The blombly compiler preloads a couple of common macros. Edit the file `libs/,bb` to change
what is preloaded in your local build. The important part for iterables is that an `in` operator
is provided. This is equivalent to preconstructing an iterator from any given data type
and running `as next` on it, and is implemented as a so-called *macro* that substitutes parts of
the code. More on macros later.

```java
while(i in 1,2,3)
    print(i);
```


## Vectors

*This segment is unders construction.*

## Maps

*This segment is unders construction.*