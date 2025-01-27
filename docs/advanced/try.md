# Errors & returns

## About

Errors in blombly are special values that can the outcome of any computation. You can identify
and handle them in your code with the `as` and `catch` statements mentioned below. But it is
also important to know that you are allowed to ignore errors. This will do nothing in your
application's "happy path", but unhandled errors will keep causing operation to fail, 
effectively cascading backwards in your program's call 
stack until they encounter code that handles them - or ignores them.

<br>

Importantly, *errors do not immediately terminate functions*. Instead,
they give you an opportunity to handle them. However, they cannot be assigned as struct fields 
or as list/map entries. On the other hand, operations like converting invalid strings to numbers, 
or using the `next` operator of iterators can return errors. 

!!! warning
    **Potential instability:** Future versions of Blombly may change whether
    unhandled errors are ignored (as happens right now) or create a failure 
    when functions end/return. They may also change how errors involved in 
    side-effects are handled.

Consider the following code in which we provide invalid console inputs.
The function `add` has float semi-types on its arguments, which means that they will
be subjected to the conversion `x|=float;y|=float;`. This is injected during compilation.
As already mentioned, an error that occurs upon this conversion does not halt the function's 
execution. Blombly is well-behaved in such scenarios, computing what it can.

```java
// main.bb
add(float x, float y) = { // converts arguments to floats
    print("Trying to add: !{x} and !{y}");
    print("This will always appear.");
    return x+y;
}
x = "First number:"|read;
y = "Second number:"|read;
z = add(x, y);
catch(z) print("Operation failed");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
First number: 1
Second number: foo
Trying to add: 1.000000 and (<span style="color: red;"> ERROR </span>) Failed to convert string to float
   <span style="color: lightblue;">→</span>  {print("Trying to add: "+str(x)+" and "+            main.bb line 1
This will always appear.
Operation failed.
</pre>


## Error handling

One way of handling errors is
the `as` keyword. This performs a normal assignment, but also returns a boolean (true/false) 
value depending on whether an error was encountered. Below is a simple one-liner that retries
reading from the console until a number is provided. 

```java
// main.bb
while(not number as "Give a number:"|read|float) {}
print(number);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Give a number: number
Give a number: 12
12
</pre>


Catch errors by using the `catch(@expression) @found else @notfound;` pattern.
This has identical syntax to conditional statements, so you can skip the alternative clause
and can enclause multiple statements in brackets. Use it to check that specific
values are not errors. This includes testing for non-missing variables. The snippet
demonstrates how the `default` statement is implemented by the standard library
by catching whether the namesake variable would be an error.

```java
// main.bb
inc(x) = {
    catch(bias) bias = 1;
    return x + bias;
}
print(inc(0));
print(inc(0 :: bias=2));
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1
2
</pre>

## Failing

You can deliberately create errors using the `fail` keyword, which accepts a string
as an argument to describe the error. This command causes currently executed functions
or methods to halt immediately and return the designated error. Below is an example.
You can also convert an existing error to a string and fail immediately.


```java
final counter = new {
    \value = 0; // private
    inc() = {this\value += 1}
    int() => this\value;
}

final safediv(x,y) = {
    if(y==0) fail("Cannot divide by zero");
    counter.inc();
    return x/y;
}

if(result as safediv(1,2)) print(result);
if(result as safediv(1,3)) print(result);
if(result as safediv(1,0)) print(result);

print("Number of safe divisions: !{counter|int}");
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
0.500000
0.333333
Number of safe divisions: 2
</pre>


The `assert @condition;` statement is a macro shipped with the language that translates to failing
upon a condition being encountered without any error message. The macro's implementation is
 `if(@condition) fail(!stringify(@condition));` and you can use it for quick checks that.
In practice, create assertions whenever you are trying to prevent operations that 
would affect hidden state from taking place if computations that folllow them would
fail.



## Try

The `@result = try{@code}` pattern intercepts errors that cannot be packed into values,
such as `fail` statements. But it also intercepts return statements that indicate successful conclusion of
computations. When entered and exited, it also waits for parallel function calls evoked in the code to conclude 
and applies all defer statements. Omit brackets when only one command is tried.


!!! warning
    **Potential instability:** Future versions of Blombly may make side-effect errors `fail` to be caught with `try`.

For example, let the interception mechanism interrupt control flow like this `sgn = try if(x>=0) return 1 else return -1;`.
A similar syntax breaks away from loops below. Contrary to errors, 
returning is lightweight to intercept. 
You could also prepend `try` to loop bodies to let internal returns skip the rest of the body - 
this would emulate other languages' *continue* just as the syntax below emulates *break*. 
Blombly does not have extra keywords to enforce only one way of interrupting execution.

<br>

Use `catch` to check on the outcome of trying, where not intercepting any return is also
considered an error. An example follows.

```java
// main.bb
start = "start:"|read|int;
end = "end:  "|read|int;
i = 0;
result = try while (i <= end) {
    i = i + 3;
    if (i >= start) return i;
}
print("Finished searching.");
catch (result) fail("Found nothing: !{result}"); // creates a custom error on-demand
print("The least multiple of 3 in range [!{start}, !{end}] is: !{result}");
```

<pre style="font-size: 80%; background-color: #333; color: #AAA; padding: 10px 20px; overflow-x: auto; overflow-x: auto;">
> <span style="color: cyan;">./blombly</span> main.bb
start: 4
end:   100
Finished searching. 
The least multiple of 3 in range [4, 100] is: 6 

> <span style="color: cyan;">./blombly</span> main.bb
start: 4
end:   1
Finished searching. 
(<span style="color: red;"> ERROR </span>) Found nothing: No error or return statement intercepted with `try`.
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  catch(result)fail("Found nothing: "+str(            main.bb line 9
</pre>


!!! warning
    The `try` statement synchronizes all concurrency when entered and exited.
    This slows down automatic parallelization, so do not overuse it.