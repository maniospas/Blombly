# Typeless OOP

Blombly aims to support object-oriented programming (OOP) patterns,
where structs created with `new` are essentially objects with attached fields
and values. At the same time, OOP is simplified to the level
of not having classes/types.

## Consistency checks

The main supposition is that struct "types"
are checked implicitly during execution by not creating errors.
To assert type compliance, write unit tests
that exemplify data accepted by various interfaces.
Operations that are expecting base primitives as inputs
can cast their inputs or outputs to the desired primitive types.
Thus, either structs are monads that can be converted to the desired base
type or an error occurs.

The following example consists of a short library,
a main file showcasing practical usage, and a test file.
There, tests ensure data type correctness by detecting
any potential deviation from expected behavior, such as if
`super.z` was wrongly typed in the addition. At the same 
time, casting to floats ensures that operations are properly
conducted.


```java
// point.bb
final Point2D = {
    str => "({this.x}, {this.y})";
    add(other) = {
        super = this;
        return new {
            Point2D:
            x = super.x|float + other.x|float;
            y = super.y|float + other.y|float;
        }
    }
    dot(other) => this.x*other.x + this.y*other.y;
}
```

```java
// main.bb
!include "point"

Coords = {
    default name = "coords";
    x = "{name}.x="|read|float;
    y = "{name}.y="|read|float;
}

a = new {Point2D:name="a";Coords:}
b = new {Point2D:name="b";Coords:}
print("a.b = {a.dot(b)}");
```

```java
// test.bb
!include "point"

ab_points = {
    a = new {Point2D:x=random();y=random()}
    b = new {Point2D:x=random();y=random()}
}

test("dot product") {ab_points:return a.dot(b)}
test("addition") {ab_points:return a+b}
```

```text
> ./blombly test.bb
[  ok  ] dot product 
[  ok  ] addition
```


*The rest of this section is under construction.*