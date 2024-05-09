# Keywords
Here is a list of keywords and symbols used by the *Blombly* language, organized into categories.

## Vector
Matrix Vector sum min max len []

```java
x = Vector(5);
i = 0;
while(i<len(x),
    x[i] = i^2;
    i = i+1;
);
print(x); // [0, 1, 4, 9, 16]
print(x/2);  // [0, 0.5, 2, 4.5, 8+]
```

## List management
List len push pop next []

```java
a = List(1, 2, 3);
print(len(a)); // 3
push(a, "any object");
print(len(a)); // 4
print(pop(a)); // "any object" (also removes it)
print(next(a)); // 1 (also removes it)
print(a[0]); // 2
```

## Operations
+ - / * ^ log == <= >= < > != int bool float str time true false numbers

```java
print(1+2); // 3
print(1-1.5); // 0.5
print(1/2); // 0.5
print(2*3); // 6
print(2^3+1); // 9
print(1==2); // false
print(1==2 || 3!=2); // true
print(1==2/2 && 2==2); // true
print(int(1/2)); // 1
print("value: "+str(1)); // value: 1
```


## Control flow
while if default new final : {} ()
