# Signals

Blombly has a success-fail mechanism to capture algorithm outcomes. This consists of two parts. First is a `try` statement, which is applied on a block of code to obtain the outcome of any internal `return` or `fail` signals; these are otherwise ways of escaping calling methods by immediately returning values or creating errors, but `try` ensures that all such signals are intercepted. Its outcome is the intercepted signal and can be assigned to a variable for further handling, like so:

```java
result = try {
    x = x + 1;  // invalid code that creates an error (missing variable x)
}
print(result);
print("Successfully intercepted the above error");
```

Similarly to other control flow statements, brackets are optional for a `try` that is applied to only one statement. Omitting brackets can be used to create concise variations of other control flow statements. For example, one can compute exiting values, break loops, or detect if loops were not broken. For example, an one-liner that computes the sign of a variable `x` is `sgn = try if(x >= 0) return 1 else return -1;`

We typically want to differentiate between those try results that hold errors and those that do not. This is achieved through the catch statement; this is near-identical to a conditional statement with the only difference being that it checks whether the condition is an error (instead of checking a boolean value). An example usage of this statement is demonstrated below. In this, the try statement is responsible for intercepting the returned value of i, which stops the loop immediately.

If no value was returned, try assigns an error value to the result.

```java
least = 4;
i = 0;
result = try while (i <= 100) {
    i = i + 3;
    if (i >= least) return i;
}
print("Finished searching.");
catch (result) fail("Found nothing: {result|str}");
print("The least multiple of 3 in range [{least|str}, 100] is: {result|str});
```

If nothing is intercepted by a try statement, then a missing value is yielded. In this case, use the as syntax like normally to identify if there is a return value. For example, the following snippet inlines an algorithm and checks that it returns a value. Remove the return statement from the algorithm code block (leave it empty) and see how the fail statement correctly creates an error.

```java
algorithm = {return "result";}

if (x as try algorithm:) 
    print(x)  // ; cannot be the last symbol before else
else
    fail("Nothing was returned");
```

