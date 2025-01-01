# Iterables

Here we cover builtin types that represent collections of elements.
These are `list`, `vector`, `map`, `range`, and the complementary `iter`
that helps loops iterate through elements. All the aforementioned
data types are called iterables because they can successfully be
typecasted into `iter` instances. 

Some [structs](structs.md) are
also treated as iterables if they overload the appropriate opreations, 
but more on this elsewhere. For now, keep in mind that converting to
iterators is done automatically when you use the `in` syntactic sugar presented
later in this page.


## Lists


<div style="background-color: rgb(159, 236, 199); color: black; text-align: center; padding: 20px 0; font-size: 24px; border: 1px solid black;">
    All list modifications are efficient; even front popping with next.
</div>

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
> ./blombly main.bb
[4, 2, 3]
3
```

You may initialize lists based on one element like below.
This notation in general converts data to lists (and 
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
> ./blombly main.bb
[5] 
( ERROR ) Out of range
   → A[3]=0                                              main.bb line 4
```

The above examples do not modify lists during execution.
To extract while removing data from lists `next` and `pop` operators 
that extract and remove the first or last list elements respectively. 

These yield error values if the list is empty, so use the `as` assignment in a loop to
extract all list elements.

```java
// main.bb
A = 1,2,3;
while(a as next(A)) 
    print(a);
```

```bash
> ./blombly main.bb
1
2
3
```


Append elements at the end using the `push` operator, like below.

```java
// main.bb
A = 1,2,3;
push(A, 4);
print(A);
```

```bash
> ./blombly main.bb
[1, 2, 3, 4]
```

Concatenate lists by adding them like below.

```java
// main.bb
print((1,2,3)+(4,5));
```

```bash
> ./blombly main.bb
[1, 2, 3, 4, 5]
```

Finally, lists can yield multiple elements at once by providing any iterable yielding integer identifiers.
Internally, lists obtain an iterator from that iterable and traverse its values like below.
Value repetitions are allowed. If the iterable yields an out-of-range value, 
sub-indexing will stop at the first missing one. 

```java
A = 1,2,3,4,5;
print(A[1,3]);
print(A[range(1,4)]);  // more on ranges later
```

```bash
> ./blombly main.bb
[2, 4] 
[2, 3, 4] 
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


An addition to other iterators, Blombly provide ranges that represent values in a specified range. 
These are iterators that can only consumed once and there are four construction
patterns for them:

- `range(int end)` creates a range from `0` up to `end-1` and step `1`.
- `range(int start, int end)` starts from the newlly specified number.
- `range(int start, int end, int step)` is similar, where the step with which values increase is also specified.
- `range(float start, float end, float step)` also handles real numbers or a mixture of real numbers and integers instead of only integers (there will be an error otherwise)

Negative steps are also allowed. Below is a demonstration.

```java
// main.bb
it = range(2, 0, -0.5);
while(i as next(it))
    print(i);
```

```text
> ./blombly main.bb
2.000000 
1.500000
1.000000
0.500000
```


## `in` 

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

Vectors correspond collections to concecutive float values of fixed size. You can perform
element-by-element arithmetic operations on them for fast scientific computations. Treat them
as lists of values whose elements can not be inserted or modified - only set. Element default
values are zeros.

Vectors can be created either from their number of zero elements or from a list of numbers.
They also support sub-indexing from iterables yielding integer identifiers. This is especially fast
when ranges are provided, as in the following example.

```java
// main.bb
x = vector(1,2,3,4,5);
y = vector(1,2,3,4,5);
z = x+y-1;
z = z[range(3)];
print(z);
```

```text
> ./blombly main.bb
[1.000000, 3.000000, 5.000000] 
```

**Current implementations run purely in the CPU, but future BlomblyVM implementation will consider scheduling GPU operations via any supported CUDA.**


## Maps

Maps contain transformations between iterable objects
that match keys to values. Use any objects or non-error primitives
as keys and values. For keys, numbers and strings keys are considered
the same based on their value, whereas other data match only themselves.

Maps implement member access and set operators. But they are also iterables
that yield `key, value` pairs as lists of two entries. Pop from the frnt or back of the pair 
with the `|` notation to extract keys and values with concise syntax that retains
clear semantics.


```java
// main.bb
A = map();
A["A"] = 1;
A["B"] = 2;
A["C"] = 3;

print("---- Pairs");
while(pair in A) print(pair);
print("---- Keys");
while(pair in A) print(pair|next); // equivalent to next(pair)
```

```text
> ./blombly main.bb
---- Pairs
[B, 2] 
[C, 3] 
[A, 1] 
---- Keys
B
C
A
```

*Blombly does not implement sets. Instead, treat those as maps from objects to `true`.*