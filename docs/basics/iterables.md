# Iterables

Here we cover builtin types that represent collections of elements that can
be iterated.
These are `list`, `vector`, `map`, `range`, and the complementary `iter`
that helps loop through the elements of the rest.
[Structs](structs.md) overloading element access and length are
also treated as iterables. Converting to
iterators before looping is done 
automatically when using the `in` statement presented later.


## Lists

Declare lists by separating values with a comma. Access and set
its elements with square brackets where element indexes
start from zero. You can also provide other iterables that hold
integer-only values to retrieve multiple elements until that
iterable goes out of bounds.
Here is an example that also demonstrates usage of
the `len` function to obtain the number of elements.

```java
// main.bb
A = 1,2,3,4;
A[0] = 100;
print(A);
print(A|len);
print(A[range(1,3)]);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
[100, 2, 3, 4]
4
[2, 3, 4]
</pre>

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

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
[5] 
(<span style="color: red;"> ERROR </span>) Out of range
   <span style="color: lightblue;">→</span>  A[3]=0                                              main.bb line 4
</pre>

## Len changes

The above examples do not modify lists during execution.
To extract data while removing them from lists use the `next` and `pop` operators
to obtain the first or last list elements respectively. 
These yield error values if the list is empty, so use the `as` assignment to detect
whether the list was empty in loops. All list modifications are efficient; even front popping with next.
Append elements at the end using the `push` operator.

```java
// main.bb
A = 1,2,3;
push(A, 4);
while(a as A|next) print(a);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
2
3
4
</pre>



Cconcatenate lists to leave the originals intact, like below.

```java
// main.bb
print((1,2,3)+(4,5));
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
[1,2,3,4,5]
</pre>



## Iterators

Given any data structure that accepts an element getter and length methods,
you can create an iterator to traverses through its elements without modifying it. 
Such data include lists and are called iterables. 
Obtain an iterator from an iterable with the `iter` typecasting as in the example below. 
Iterators admit only the `next` operator, and always accept a conversion to `iter` that
returns themselves.
One cannot modify data, perform random access, or restart traversal of the iterable's elements. 
Create a new one to go through data that are already consumed.

```java
// main.bb
A = 1,2,3;
it = iter(A);
while(i as it|next) print(i);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
2
3
</pre>

!!! info
    Iterators are safe in that they do not crash while the data structure they are iterating
    through is modified, for example by adding, changing, or removing elements. The iterator
    may miss parts of the modifications it has already traversed through.

The blombly compiler preloads a couple of common [macros](../advanced/preprocessor.md) in `libs/,bb`.
The important part for iterables is that an `in` operator
is provided. This preconstructs an iterator from any given data type
and runs `as next` on it. Here is an example.

```java
A = 1,2,3;
print("Original length {A|len}");
while(i in A) print(i);
print("Length after iteration {A|len}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Original length 3
1
2
3
Length after iteration 3
</pre>

## Ranges

An addition to other iterators, Blombly provide ranges that go through a specified range
of numbers. Like all iterators, ranges are only consumed once and need to be created anew to be
repeated. There are four construction patterns for them.

- `range(int end)` creates a range from `0` up to `end-1` and step `1`.
- `range(int start, int end)` starts from the newly specified number.
- `range(int start, int end, int step)` is similar, where the step with which values increase is also specified.
- `range(float start, float end, float step)` also handles real numbers or a mixture of real numbers and integers instead of only integers (there will be an error otherwise)

Negative steps are also allowed. Below is a demonstration.

```java
// main.bb
it = range(2, 0, -0.5);
while(i as next(it)) print(i);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
2.000000 
1.500000
1.000000
0.500000
</pre>


## Vectors

Vectors correspond collections to consecutive float values. They differ from lists
in that they hold floats and have a fixed size. This known information is leveraged
to provide performant implementations of scientific computations.

<br>

Perform element-by-element arithmetic operations on them for fast scientific computations. Treat them
as lists of values whose elements can not be inserted or modified - only set. Element default
values are zeros.
Vectors can be created either from their number of zero elements or from a list of numbers.
They also support sub-indexing from iterables yielding integer identifiers. This is especially fast
when ranges are provided, as in the following example.

```java
// main.bb
x = vector(1,2,3,4,5);
y = vector(1,2,3,4,5);
x[1] = 1; // modify element
z = x+y-1;
z = z[range(3)];
print(z);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
[1.000000, 2.000000, 5.000000] 
</pre>


## Maps

Maps contain transformations between iterable objects
that match keys to values. Use any objects or non-error primitives
as keys and values. For keys, numbers and strings keys are considered
the same based on their value, whereas other data match only themselves.
Maps implement member access and set operators. But they are also iterables
that yield `key, value` pairs as lists of two entries. Pop from the front or back of the pair 
with the `|` notation to extract keys and values with concise syntax that retains
clear semantics. Finally, concatenate maps by adding them.


```java
// main.bb
A = map();
A["A"] = 1;
A["B"] = 2;
A["C"] = 3;

print("---- Pairs");
while(pair in A) print(pair);
print("---- Keys");
while(pair in A) print(pair|next);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
---- Pairs
[B, 2] 
[C, 3] 
[A, 1] 
---- Keys
B
C
A
</pre>

!!! tip
    Blombly does not implement sets. Instead, treat those as maps from objects to `true`.