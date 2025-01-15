# Typeless OOP

Blombly aims to support object-oriented programming (OOP) patterns,
where structs created with `new` are essentially objects with attached fields
and values. At the same time, OOP is simplified to the level
of not having classes/types.

## Semi-typing

The main supposition is that struct "types"
are checked implicitly during execution by not creating errors.
To assert type compliance, write unit tests
that exemplify data accepted by various interfaces.
Operations that are expecting base primitives as inputs
can cast their inputs or outputs to the desired primitive types.
This way, structs serve as monads that can be converted to
the base type - or create an error in the process.

The following example consists of a short demonstration
of those concepts. In particular it acknowledges
that -in the whole program- it would make sense for
`other.x` and `other.y` to be more complicated structs instead 
of a float. Ignoring what data they may actually contain,
it tries to enforce a conversion into a float.
This ensures that the `other`
struct is converted the needed format and moves the burden
of the conversion to the latter.


```java
// point.bb
final Point2D = {
    str => "({this.x}, {this.y})";
    add(other) => new {
        Point2D:
        x = this..x + other.x|float;
        y = this..y + other.y|float;
    }
    dot(other) => this.x*other.x|float + this.y*other.y|float;
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

## Testing

Something to consider is that

```java
// test.bb
!include "point"

ab_points = {
    a = new {Point2D:x=random();y=random()}
    b = new {Point2D:x=random();y=random()}
}

test("dot product", 10) {ab_points:return a.dot(b)}
test("addition", 10) {ab_points:return a+b}
```

```text
> ./blombly test.bb
[  ok  ] dot product 
[  ok  ] addition
```
