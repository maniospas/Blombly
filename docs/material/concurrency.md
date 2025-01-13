# Concurrency

A key characteristic of Blombly is that all functions and struct methods have a chance 
of being executed concurrently if they do not take each other's outputs as inputs. 
For example, consider the following code snippet
where print statements are called from within a function; they end up occurring
non-sequentially. This lets the language parallelize running tasks, 
and in this page we will show how to tame similar side effect that are not part of
the business logic. Discussed concepts can be found throughout the user guide, but here
they are organized into one place.


```java
// main.bb
printer(text) = {print(text)}
printer("A");
printer("B");
printer("C");
printer("D");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
B 
D 
A 
C 
</pre>


!!! info
    Blombly's execution model is similar to lazy evaluation, with the difference that side
    effects are allowed.

!!! warning
    Concurrency is *not* guaranteed. It may happen under the hood by an internal scheduler
    to speed up execution, but you should never write code as if it were concurrent.
    You should only anticipate issues due to side effects.

## Synchronization

Blombly makes asynchronous code that returns a value
complete before that value is used in subsequent code. This lets you think of code as sequential
when there are no side effects (see below). For example, the following snippet
produces the expected result, given that the first two function calls run concurrently.
But the final result is retrieved for the last call.

```java
// main.bb
add(x,y) => x+y;
x = add(1,2);
y = add(3,4);
print(add(x,y));
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
10
</pre>

As an extension of dependency-based synchronization mechanisms, `try` statements also
force all internally executed code to conclude before returning because they need to
be aware of any errors that have occurred. Below is an adjustment of the first concurrency
example, where the `try` keyword is prepended to function calls to make them conclude
before moving on. 
This does not effect error handling, because unhandled errors are reraised at the end
of the scope even if intercepted.

```java
// main.bb
printer(text) = {print(text)}
try printer("A");
try printer("B");
try printer("C");
try printer("D");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
A
B
C
D
</pre>



## Side effects

Concurrency is an issue when function calls exhibit so-called side effects.
These are data or input/output changes that occur as a by-product of the business logic
and therefore no controlled with synchronization mechanism. Aside from internal 
implementation of details of the virtual machine that maintain safety, side effects
cannot be controlled with inherently problem constructs like locks that may hang
execution. Instead, you need to rely on either sycnhronization or -when you are working
with data structures- ownership and method atomicity discussed next.

<br>

There are three main kinds of side effects:

1. Modifying struct fields.
2. Pushing, popping, or setting elements to lists or maps.
3. IO operations, such as conole read/writes, the file system, and web resources. 

IO may be modified externally too. But if concurrency is properly acknowledged throughout
code writing, you would not write code for such operations any differently.

!!! info
    Built-in methods (like `push` and `pop`) are both safe to execute and run
    sequentially. This holds true even when overloaded.


## Method atomicity

Struct methods run atomically. That is, they guarantee that the fields of `this` cannot be accessed or 
modified concurrently. That said, they can call functions or methods that have side-effects
in the struct. Such calls -including dependent calls of the same struct's methods- are fully synchronized. 
Guarantees on not modifying the
struct through concurrency not apply to side effect types #2 and #3 mentioned above.
They also do not apply to other structs, which may be modified even between successive calls to them.

```java
accum = new {
    value = 0;
    add(x) = {this.value += x; print("added {x}  sum {this.value}")}
}

try while(i in range(10)) accum.add(i);
print(accum.value);
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
added 1  sum 1 
added 3  sum 4 
added 0  sum 4 
added 4  sum 8 
added 5  sum 13 
added 2  sum 15 
added 6  sum 21 
added 7  sum 28 
added 8  sum 36 
added 9  sum 45 
45 
</pre>
