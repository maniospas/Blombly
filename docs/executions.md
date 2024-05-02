# Execution modes

**Inline:** The code block runs sequentially and immediately within the current scope. Also used by all conditions and loops. Examples of inline calling and conditional statements:

```java
final inc = {
    result = result+1;
    return result;
}
final add = {
    result = x+y;
    inc:
    print("this line is never reached");  // never printed due to the return within inc being executed in-line
}
print(add(x=1, y=2));  // 4
```


**New:** The code block runs sequentially in a new scope that inheritst the previous one but also keeps new assignments. Creates a *this* variable to be returned. Also used when passing arguments to calls. Any returns change the outcome of new but do not affect the parent block's execution. 

```java
a = 1;
obj = new(
    this.x = a+1;
);
print(obj.x);  // 2
```

**Call:** The code block runs in a thread in parallel. It first runs the arguments and passes those. The running block can see only the final variables from the scope where it is called. Waiting for the thread to
conclude is only done if its result is used.


```java
a = new(
    this.x = 0;
    inc = {
        default value=1; 
        this.x = this.x+value;
    }
    sync = {return this;}
);
a.inc(value=2);
print(a.x); // still 0 probably because inc is running in parallel
a = a.sync(); // this pattern forces synchronization
print(a.x); // 2
```