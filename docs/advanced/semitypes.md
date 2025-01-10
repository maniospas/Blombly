# Semi-types

The last aspect of the blombly language is dynamic typecasting that reinterprets
data as different formats. The purpose of this mechanism is to ensure that variables contain
or are transformed to appropriate data, therefore introducing type correctness guarantees
for subsequent code. At its core, this is equivalent to expressing
method calls of one argument. However, code writting is drastically simplified:

- Conversions are chained using fewer symbols and less nesting.
- Conversions are read as variable type semantics, which we dub semi-types.

## Type conversion

The main notation is a vertical slash per `expression | convertion`,
which in a vacuum is equivalent to calling `conversion(list(expression))`. 
This has lower priority than commas to denote lists because the goal is
to apply conversion to speficic variables being accessed. Here is an example that converts an integer 
to a float by leveraging the builtin typecasting mechanisms:

```java
// main.bb
x = 1|float;
print(x);
```

```text
> ./blombly main.bb
1.000000
```

Observant readers may have already noted that conversion mechanisms are structured
in a way that does not allow for executable code in the arguments (reminder that argument
code is a generalization of keyword arguments).
To add parameters to the conversion callable, create it as a struct like below.

```java
final fmt_generator(specs) => new{
    // make the returned struct a callable that applies the numeric formatting pattern
    specs = specs|str; // ensure str type for specs while bringing them within new
    call(x) => x[this.specs]; 
}

print(1.2345 | fmt_generator(".3f"));
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
1.234
</pre>



## Implicit types

Similarly to numeric operations, the expression `variable |= convertion;` reassigns to a variable. 
In this case, however, the leftwise convertions is performed first, enabling the pattern
`variable |= convertion1|convertion2|...;` 
This notation is intentionally similar to 
[double turnstile](https://en.wikipedia.org/wiki/Double_turnstile) and may be thought as 
variable modeling some property. Given the automatic conversion of casted variables, 
create methods that transform their arguments to a format they can parse.
Below is an example.

```java
final nonzero(x) = {
  if(x==0) fail("Zero not allowed"); 
  return x;
}

final safediv(x, y) = {
  x |= float;
  y |= float|nonzero;
  return x / y;
}

print(safediv("1", 2));
```

<pre style="font-size: 80%;background-color: #333; color: #AAA; padding: 10px 20px;">
> <span style="color: cyan;">./blombly</span> main.bb
0.5
</pre>

## Type overloading

In theory, you can handle various scenarios of your data in a function
that is used as a semi-type. However, most problems require some kind
of adjustment on their input data. Since blombly intentionally avoids 
reflections or a hierarchical type system, whar you can do instead is
create a semitype that evokes respective methods of structs.
Here is an example:

```java
safenumber = {
  nonzero = {if(this.value==0) fail("zero value"); return this.value}
  float => this.value;
}

x = new{safenumber:value=1}
y = new{safenumber:value=0}

semitype nonzero; // declares the existence of the semitype for next code

ratio = try x|float / y|nonzero;  // immediately intercept the value
print(ratio|str);
```

