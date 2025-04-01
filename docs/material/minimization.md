# Minimization

<i>by Emmanouil Krasanakis - 2025/4/1</i>

<br>

In this article, I will deconstruct Blombly's freshly-completed code minimization tricks.
I found these fun to think about and simple to implement. So maybe they can serve as 
inspiration during the development of other languages too.

## Introduction

Code minimization is a pivotal aspect of Blombly because it supports a
convenient mantra about reusing code:

!!! tip
    Include whatever, don't worry about bloat.

For example, the virtual machine defaults to including the whole standard
library to all programs for ease of development. 
But you will at most see only relevant parts (or none of them)
in the compiled intermediate representations.
How does the compiler achieve this? Let's start with the
example below and see how far we can get. 


```java
// main.bb
A = new {
    numadd(x,y) => x+y;
    nummul(x,y) => x*y;
}

B = new {
    numadd(x,y) => x+y;
    nummul(x,y) = {fail("No mul")}
}

C = new {
    affineadd(x,y) => x+y+1;
}

print(A.numadd(1,2));
print(B.numadd(1,2));
```

<details><summary>Quick syntax refresher.</summary>
The snippet declares some structs, each equipped with a couple of methods. <code>=&gt; value</code> 
is used as a shorthand for <code>= {return value}</code>.
</details>

## No reflection

By design, Blombly does not implement object-oriented reflection,
which would be the ability to inspect source code during runtime.
Of course, as in all dynamic languages, you can access struct fields like
`A.field` and get error messages on missing entries.
But you *cannot* get a list of fields while programs are running. This is an
opinionated feature for preventing dynamic workarounds to static
typechecking. In my experience with dynamic languages, 
reflection is a recipe for ... spaghetti ... code. 🍝 😛
If you want to prepare for different variable types, 
statically typed languages is what you are looking for. And if you want
to have key-value pairs, dictionaries work just fine.

<br>

On the other hand, lack of reflection means that we are free to remove unused struct fields. 
For instance, reused code may define a broad range of features, but it better to only keep those 
that are actually used. 
In the following example, computing the sums of `norm2d` takes up 2/3rds of the memory 
compared to summs of `norm3d` because there is no need to keep track of the `z` coordinate of points. 
Relevant for here, this optimization also means smaller intermediate representations, for example
by not including the implementation for one of the norms.

```java
Point = {norm2d()=>(x^2+y^2)^0.5; norm3d()=>(x^2+y^2+z^2)^0.5}
points = list();
while(i in range(10000)) points << new {Point:x=i;y=i;z=i}
sum_norms = 0;
while(point in points) sum_norms += point.norm2d();
print(sum_norms);
```

To identify code segments to be removed,
the compiler runs an inference engine based on transitive usage properties.
That is, if a variable with name `A` uses a variable with name `x`, 
then the name `x` will be considered "in use"
as long as the name `A` is also in use. At the end of the usage chain, 
function returns, and values printed to the console, files, graphics, 
the web, databases, etc are considered in use.

## Symbol removal

To implement an algorithm that removes lots of useless code, 
go through all symbols and count the number of
usages. For example, the *bbvm* (Blombly virtual machine) instruction `add z x y`, which 
is the intermediate representation for `z=x+y;`, increases the usage
count for the last two symbols.

<br>

In a related note, functions like `numadd(x,y) => x+y;`
are translated during parsing to simple code block declarations like this
`numadd = {x = next(args); y=next(args); return x+y}` (code blocks do not run
on declarations - they are called as functions or inlined as pure code).
So, even functions/methods and struct creation code are but simple assignments.
Minimization is therefore just a matter of pruning unused symbol names.
Not all useless *variables* will be removed, because the same
symbols may be repeated throughout many scopes. But at least we get
rid of large chunks of code without losing dynamism.

<br>

Repeat the above until nothing is removed anymore. This typically 
runs in amortized time *O(n log n)*. In practice,
several minimizations repetitions across hierarchical
data structures lead to only a few redundant stuff slipping through. 
Everything is applied directly on the intermediate
representations, but to avoid looking at a ton of instructions let's say 
that it would be equivalent to write the following snippet. 

```java
// main.bb
A = new {numadd(x,y) => x+y}
B = new {numadd(x,y) => x+y}

print(A.numadd(1,2));
print(B.numadd(1,2));
```

<details><summary>Amortized time analysis.</summary>

This process requires <i>O(n)</i> running time
to go through the code once, where *n* is the number of symbols and can be
considered proportional to code size, be it number of lines, tokens, etc. 
This means that at worst we get <i>O(n<sup>2</sup>)</i> amortized running
time if all symbols depended on each other and we needed to remove all of them,
one at a time. But a more realistic case is
<i>O(n log n)</i> if we consider that, on average, there will be more than one
independent computations that can be removed together. In this case,
the algorithm is pretty scalable.

</details>

!!! warning
    If you implemented methods called 
    `add` and `mul`, these might not be optimized away given that they 
    could represent operator overloading. Future versions could be more clever
    in that front, but I don't plan to spend a lot of time on this edge case
    because I expect the gains to be non-existent for most programs.


## Definition cache

To further minimize the produced code, another inference engine identifies duplicate 
code blocks and creates shared definitions among them. These are loaded at the beginning
of programs in a symbol cache and can be retrieved by scopes without having to go through
chains of parent scopes. This retrieval is even faster than normal because there is no
need to check for waiting threads. 

<br>

To identify equivalent code blocks in the simplest case,
named variables are considered the same only if they have the same name. This 
restriction is applied because code blocks could be inlined within function calls 
or struct creation. Future implementations can seek to infer blocks that will 
never be inlined and let those anonymize some internal variables.

<br>

In addition to fixed variables, though, 
parsing also creates intermediate ones. For example, the expression 
`return x+y` is normally compiled to the following instructions. The first
argument is the returned value of each operation, with *#* indicating no
such value. 

```bash
add _bb0 x y
return # _bb0
```

Intermediate values like *_bb0* above are created with an automatically
increasing counter, but we would like to structurally align them when 
matching code blocks. Abstract Syntax Tree (AST) isomorphism checks are tempting,
but in my opinion needlessly complex when there are much faster alternatives.
Instead, Blombly's optimizer internally replaces intermediate variables 
with the representation [_bbpass][XXH3][local counter] and serializes
each code block to a string. This lets it use a hashmap with *O(1)* lookup
of whether an equivalent implementation already exists - a huge advantage 
over isomorphism checks that would need to compare implementations pairwise.

<br>

In the variable representation, XXH3 is a fast 128-bit hash function from the
[xxhash](https://github.com/Cyan4973/xxHash) library applied on the rest of the
first instruction setting the variable.
The hash is converted to hex format but not to 8 chars because we need
it to remain printable for debugging. Finally, the local counter resets at the 
start of each code block. Variables starting with *_bbpass* and those indicating 
code blocks already added to the cache are not further anonymized with this scheme.

<details><summary>Probability analysis</summary>

The above mechanism guarantees equality for
identical variables, and therefore the same code blocks will produce matching implementations.
On the other hand, hash collisions have probability 2<sup>-64</sup>. 
In the end, the probability of collisions
at least one wrong parallelization is less than 2<sup>-64</sup>C<sup>2</sup>, 
where *C* is the total number of code blocks (collisions need to happen at the same block
position). If the parallelization
scheduler ever got less than 20% mistakes I would already be very happy, so you can imagine
that I am not worrying at all about this probability; it starts becoming noticeable (exceeds 0.4%) 
after 2<sup>28</sup>=268,435,456 variables. This is well beyond the scale of the projects
I aim to support with the language and for this reason I am considering 64-bit hashes
in case their string conversion proves to be a compilation bottleneck.

</details>

The outcome of also running the caching mechanism is presented at
the end of this article. Unfortunately, optimizations occur at the intermediate representation level, 
which lacks one-to-one correspondence with Blombly code. 
(One of the virtual machine's goals is that you should not worry about optimizations anyway, so the language
is focused on removing anti-patterns instead of giving low-level access to how operations execute.)

<br>

To get a taste of what caching is all about, the underlying implementation is conceptually very similar 
to what ends up being created.
The important part is that common code segments are abstracted away. This may look more complicated
when we are looking at Blombly code, but it's already tens of *bbvm* instructions less when implemented.
Importantly, it scales well if we had much duplication, as is the case when the same files are imported
multiple times throughout a project.
We iterate the cache generation process until convergence, starting from a cache 
of larger code blocks/functions and moving internally. Similarly to previous analysis,
and as long as we are calling more than one method or have a method plus business logic
in each function on average, the amortized running time becomes *O(n log n)*.

```java
cache2 = {
    x=next(arguments);
    y=next(arguments);
    return x+y;
}
cache0 = {numadd=cache2} // bbvm instructions also pack returning the object here
cache1 = {arguments=1,2} // for safety, the compiler does not allow us to set to the name `args`

// `new cache0` is the underlying bbvm instruction (calling new on a code block)
// but that pattern is disabled in the main language to prevent anti-patterns
A = new {cache0:} 
B = new {cache0:}

print(A.numadd(cache1:));
print(B.numadd(cache1:));
```

As an example, here is cache2's implementation under-the-hood. Notice the hash in the representation.


```bash
BEGIN _bbcache2
next x args
next y args
add _bbpassc3bdd6e4dddf22c0e0bb321371f878b90 x y
return # _bbpassc3bdd6e4dddf22c0e0bb321371f878b90
END
```

!!! info
    Optimization performs a preliminary pass that compresses some parsing outcomes, 
    such as substituting macro symbols that are matched to variables instead of
    holding whole expression outcomes.


## Final representation

The final *bbvm* code is presented below. This is produced through execution with 
the `-s --text` options to respectively strip away debugging symbols
and not compress the text to keep it readable. 
A last step that the compiler applies before generating this code is 
to re-parse everything so that intermediate
variables retain their naming scheme but are unified under a new global counter.
This prevents the large number of characters in hashes from bloating the final file. 

<br>

Before closing, notice the absence of the *C* struct and of both *nummul*
implementations that were optimized away at the first step.
There are also two cache segments, each corresponding to a different repetition
of code block minimization. All in all, I rather like how the whole mechanism 
shaped up.

<br>

That's all for now. 🙂 <br>
I hope you found the discussion fun. Happy coding!


```bash
./blombly main.bb -s --text
3
3

cat main.bbvm

CACHE
BEGIN _bb0
next x args
next y args
add _bb1 x y
return # _bb1
END
END
CACHE
BEGIN _bb2
ISCACHED numadd _bb0
return # this
END
BEGIN _bb3
BUILTIN _bb4 I1
BUILTIN _bb5 I2
list::element args _bb4 _bb5
END
END
ISCACHED _bb6 _bb2
new A _bb6
ISCACHED _bb7 _bb2
new B _bb7
ISCACHED _bb8 _bb3
get _bb9 A numadd
call _bb10 _bb8 _bb9
print # _bb10
ISCACHED _bb11 _bb3
get _bb12 B numadd
call _bb13 _bb11 _bb12
print # _bb13
```