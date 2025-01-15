# Errors & returns

## As

Computations like converting invalid strings to numbers, or using the `next` operator of iterators,
return errors as values. These make execution fail if used in computations.
One way of handling errors is
the `as` keyword. This performs an assignment without breaking normal code writing 
upon encountering an error value, but returns a true/false value depending on whether
an error was found. Below is a simple one-liner that retries
reading from the console until a number is provided.

```java
while(not number as "Give a number:"|read|float) {}
print(number);
```


<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
Give a number: number
Give a number: 12
12
</pre>

## Try

The `@result = try{@code}` pattern intercepts errors that indicate
unsuccessful algorithms, as well as return statements that indicate successful conclusion of
computations. When entered and exited, it also waits for parallel function calls evoked in the code to conclude first 
and applies all defer statements. Omit brackets when only one command is tried.

<br>

For example, let the interception mechanism interrupt control flow like this `sgn = try if(x>=0) return 1 else return -1;`.
A similar syntax breaks away from loops below. Contrary to errors, 
returning is lightweight to intercept. 
You could also prepend `try` to loop bodies to let internal returns skip the rest of the body - 
this would emulate other languages' *continue* just as the syntax below emulates *break*. 
Blombly does not have extra keywords to enforce only one way of interrupting execution.

<br>

You may further want to differentiate between try results that hold errors and those that do not. 
In those cases, use `catch` to check the outcome of trying, 
which is effectively a special conditional statement that checks whether the condition is an error.
Missing values are not considered errors for the purposes of this statement. 
Usage is demonstrated below, where the return signal is intercepted to stop the loop immediately. 
If no value or error is intercepted, the result becomes a missing value error that can be caught.
Catching errors does not need to occur immediately either.

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

!!! tip
    The `as` statement should be preferred for error handling.
    Use `try` only to intercept returns or to keep the program running upon unforeseen errors.

!!! warning
    The `try` statement synchronizes all concurrency in scope both when entered and when exited.
    This may reduce parallelization speedups, so do not overuse it.