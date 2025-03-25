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
their elements with square brackets where element indexes
start from zero. You can also provide as indexes 
lists or other iterables that hold integer-only values.
This retrieves multiple elements until the key's index
goes out of bounds.
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
(100, 2, 3, 4)
4
(2, 3)
</pre>

You may initialize lists based on one element like below.
This notation in general converts data to lists of one element.
Otherwise, using a list a semit-type forces a conversion from
other iterables.

<br>

An error is returned from functions that write on out-of-bounds
elements, and a missing value error is returned when trying
to access such elements. 
Use the `as` assignment to check for validity if list size is unknown,
as errors are returned from functions that would add errors
to lists instead of doing so.

```java
// main.bb
A = list::element(5);
print(A);
A[3] = 0; // CREATES AN ERROR
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
(5) 
(<span style="color: red;"> ERROR </span>) Out of range
   <span style="color: lightblue;">→</span>  A[3]=0                                              main.bb line 4
</pre>

**Modifying len**

The above examples do not modify lists during execution.
To extract data while removing them from lists use the `next` and `pop` operators
to obtain the first or last list elements respectively. 
These yield error values if the list is empty, so use the `as` assignment to detect
whether the list was empty in loops. All list modifications are efficient; even front popping with next.
Push elements to the end using the `<<` operator.

```java
// main.bb
A = 1,2,3;
A << 4;
while(a as A|next) print(a);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
2
3
4
</pre>


**Concatenation**

Cconcatenate lists to leave the originals intact, like below.

```java
// main.bb
print((1,2,3)+(4,5));
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(1,2,3,4,5)
</pre>

!!! tip
    Pushing to lists returns the list itself, for example to enable syntax like `A = list()<<1<<2<<3;`.


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

**in**

The blombly compiler preloads a couple of common [macros](../advanced/preprocessor.md) in `libs/.bb`.
The important part for iterables is that an `in` operator
is provided. This preconstructs an iterator from any given data type
and runs `as next` on it. Here is an example.

```java
A = 1,2,3;
print("Original length !{A|len}");
while(i in A) print(i);
print("Length after iteration !{A|len}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Original length 3
1
2
3
Length after iteration 3
</pre>

<br>

**!gather**

A second macro designed for comprehensive iterable manipulation is `!gather(@init, @aggregate) {@body}`. 
The exclamation mark is treated as a normal character and is purely cosmetic, but
it lets you know that the macro affects control flow. This macro contains an
initialization statement, a binary self-modification operator that is applied on
every new element, and a body whose return statements are modified to signify what
is gathered. Here is an example of how to filter through an iterable's elements:


```java
A = 1,2,3,4,5,6;
B = !gather(list(),<<) {while(x in A) if(x%2==0) return x;}
print(B);
```
<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(2,4,6)
</pre>



## Ranges

Blombly provides ranges that go through a specified set
of numbers. Like other iterators, ranges are only consumed once and need to be created anew to be
repeated. There are four construction patterns shown below. You can also
construct an iterator that produces random numbers ad infinitum per `random(seed)` where the seed 
is either an int or a float. Those numbers will be in the range `[0,1]`. 

| Function Signature                          | Description |
|---------------------------------------------|-------------|
| `range(int end)`                            | Creates a range from `0` up to `end-1` with a step of `1`. |
| `range(int start, int end)`                 | Starts from the specified `start` number and goes up to `end-1`. |
| `range(int start, int end, int step)`       | Similar to the previous version but allows specifying the step size. |
| `range(float start, float end, float step)` | Supports real numbers or a mix of real and integer numbers (error otherwise). |
| `random(seed)` | Generates an iterator that yields uniformly random numbers in the range `[0,1]`. |


Negative steps are also allowed. Below is a demonstration.

```java
// main.bb
it = range(2, 0, -0.5);
while(i as it|next) print(i);
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
in that they hold only floats and have a fixed size. This known information is leveraged
to provide performant implementations of scientific computations.

<br>

Perform element-by-element arithmetic operations on them for fast scientific computations. Treat them
as lists of values whose elements can not be inserted or modified - only set. Element default
values are zeros.
Vectors can be created either directly, or be converted from a list of numbers.
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
(1.000000, 2.000000, 5.000000)
</pre>


**Direct allocation**

Convert any iterable to a vector with the above syntax, provided that it contains only floats.
Initialize vectors of zero elements or -for even faster allocation-
no set values. This is done like below. Recall that `::` is treated as a normal name character
by the language.

```java
zeros = vector::zero(1000);
print(zeros|len, zeros|sum);

unset = vector::alloc(1000); // this has memory garbage
print(unset|len, unset|sum);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
(1000, 0.000000)
(1000, 235856725974...)
</pre>


## Maps

Maps associate struct or non-error primitive keys to values. 
Set their elements or provide an iterable of `(key, value)` pairs,
that is, a list of two elements. For keys, numbers and strings are considered
the same based on their value, whereas other data match only specific objects.
Maps implement member access and set operators. But they are also iterables
that yield `key, value` pairs as lists of two entries. Pop from the front or back of the pair 
with the `|` notation to extract keys and values with concise syntax that retains
clear semantics. Finally, concatenate maps by adding them.


```java
// main.bb
A = map(
    ("A", 1), 
    ("B", 2), 
    ("C", 3));
A["D"] = 4;

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
[D, 4] 
---- Keys
B
C
A
D
</pre>

!!! tip
    Blombly does not implement sets. Instead, treat those as maps from objects to `true`.
