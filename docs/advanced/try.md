# Errors & returns

## About

Errors in blombly are special values that can be found in place of any computation outcome. You can identify
and handle them in your code with the `as` and `catch` statements mentioned below. Ignore this option 
if you are only interested in your application's "happy path". Unhandled errors cascade to dependent code,
including within returned values from functions and methods.

<br>

*Most errors do not immediately terminate functions.* 
This gives you a window to handle them. For example,
operations like converting invalid strings to numbers, or using the `next` 
operator of iterators may return errors in certain situations that you can handle.
The few cases where functions are immediately terminated comprise attempts at modifying invalid
data, such as pushing to a non-existing variable, or trying to set error values
to struct fields, list elements, etc. In those cases,
errors are directly returned; catch those either at one level higher
or with the `do` clause shown at the bottom of this page.

<br>

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
(<span style="color: red;"> ERROR </span>) Failed to convert string to float
   <span style="color: lightblue;">→</span>  {print("Trying to add: "+str(x)+" and "+            main.bb line 1
This will always appear.
Operation failed.
</pre>

!!! tip
    The main error handling pattern is to check whether a function returns an error.


## Error handling

One way of handling errors is through
the `as` keyword. This is similar to assignment with `=`, but also returns a bool that indicates
whether an error was avoided or encountered (true or false respectively).
Below is a simple one-liner that retries reading from the console until a number is provided. 

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


Catch errors without assigning them eslewhere with the `catch(@expression) {@found} else {@notfound}` 
pattern. This has identical syntax to conditional statements, so you can skip the brackets or 
the alternative clause. Use it to check that specific
values are not errors. This includes testing for non-missing variables. The snippet below
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

You can deliberately create errors using the `fail` keyword.
This accepts a string message and causes currently executed functions
or methods to halt immediately and return an error. Below is an example.


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


The `assert @condition;` statement is a macro shipped with the language that fails
when a condition is not met without any error message. The macro's implementation is
 `if(@condition) fail(!stringify(@condition));` and you can use it for quick checks.
In practice, create assertions whenever you are trying to prevent operations that 
would affect hidden state from taking place if computations that folllow them would
fail.



## Do-return

The `@result = do{@code}` pattern intercepts return statements. We left it for here
because it also intercepts errors that would cause functions to fail.
When entered and exited, this pattern also waits for parallel function calls to conclude 
and applies all defer statements. Omit brackets when only one command is tried. You may also not assign
the result anywhere.

!!! tip
    Think of `do` as a function call that affects the scope. 
    
For example, let the interception mechanism interrupt control flow like this `sgn = do if(x>=0) return 1 else return -1;`.
A similar syntax breaks away from loops below. Contrary to errors, 
returning is lightweight to intercept. 
You could also prepend `do` to loop bodies to let internal returns skip the rest of the body - 
this emulates other languages' *continue* just as the syntax below emulates *break*. 
Blombly does not have extra keywords to enforce only one way of interrupting execution.

<br>

Use `catch` to check whether `do` intercepted a non-error ereturn; not intercepting any return is also
considered an error. An example follows.

```java
// main.bb
start = int("start:"|read);
end = int("end:  "|read);
i = 0;
result = do while(i <= end) {
    i = i + 3;
    if(i >= start) return i;
}
print("Finished searching.");
catch(result) fail("Found nothing: !{result}"); // creates a custom error on-demand
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
(<span style="color: red;"> ERROR </span>) Found nothing: No error or return statement intercepted with `do`.
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  fail("Found nothing: "+str(result|str)+"            main.bb line 9
   <span style="color: lightblue;">→</span>  catch(result)fail("Found nothing: "+str(            main.bb line 9
</pre>
