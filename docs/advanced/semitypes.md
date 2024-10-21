# Semi-types

The last aspect of the blombly language is dynamic typecasting that reinterprets
data as different formats. The purpose of this mechanism is to ensure that variables contain
or are transformed to appropriate data, therefore introducing type correctness guarantees
for subsequent code. At its core, this is equivalent to expressing
method calls of one argument. However, code writting is drastically simplified:

- Conversions are easily chained using much fewer symbols.
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

```bash
>blombly main.bb
1.000000
```

Observant readers may have already noted the lack of executable code in the arguments
(recall that this is a generalization of the concencept of keyword arguments). There is also a lack of automatic unpacking
to make the conversion mechanism callable with our notation by accepting only one argument.
To add parameters to the conversion create is as a callable struct, like below:

```java
final fmt_generator(specs) = {
  return new{
    specs = specs|str;       // ensure str type for specs while bringing them within new
    \call(x) = {             // make the returned struct a callable
      return x[this.specs];  // apply the numeric formatting pattern
    }  
  }
}

print(1.2345 | fmt_generator(".3f"));
```


## Implicit types

Similarly to numeric operations, the expression `variable |= convertion;` reassigns to a variable
after applying the convertion. All reassignment operators replace source code tokens and
that leftwise convertions are performed first, which in this case enables `variable |= convertion1|convertion2|...;` 
This notation is intentionally similar to 
[double turnstile](https://en.wikipedia.org/wiki/Double_turnstile) and may be thought as 
variable modeling some property.

Given the automatic conversion of casted variables, 
create methods that transform their arguments to a format they can parse.
Here is an example:

```java
final nonzero(x) = {
  if(x==0) 
    fail("Zero not allowed"); 
  return x;
}

final safediv(x, y) = {
  x |= float;
  y |= float|nonzero;
  return x / y;
}

print(safediv("1", 2));
```

```bash
>blombly main.bb
0.5
```