# Typeless OOP

Blombly aims to support object-oriented programming (OOP) patterns,
where structs created by `new` are essentially objects with attached fields
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

<br>

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
    str => "(!{this.x}, !{this.y})";
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
    x = float("!{name}.x="|read);
    y = float("!{name}.y="|read);
}

a = new {Point2D:name="a";Coords:}
b = new {Point2D:name="b";Coords:}
print("a.b = !{a.dot(b)}");
```

## Testing

Blombly offers a simple testing macro that allows you
to run a unit or integration test one or more times.
There is also an assert statement as a shorthand for
checking a condition and failing.
Write code so that its inputs can always be randomized
by tests. When it comes to random numbers, use a seeded
version of distributions to ensure test replicability.

```java
// test.bb
!include "point"

ab_points = {
    rand = random(42); // seeded random generator
    a = new {Point2D:x=next(rand);y=next(rand)}
    b = new {Point2D:x=next(rand);y=next(rand)}
}

final repetitions = 10;
test("dot product", repetitions) {
    ab_points:
    c = a.dot(b);
    assert c>=-2 and c<=2;
}
test("addition", repetitions) {
    ab_points:
    c = a+b;
    assert a.x+b.x == c.x;
    assert a.y+b.y == c.y;
}
```

```text
> ./blombly test.bb
[  ok  ] dot product 
[  ok  ] addition
```
