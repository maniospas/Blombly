# Language Design

Here is a summary of Blombly's design and philosophy going forward.
The main point is that the language should stay simple, with emphasis on easily
reusing code without wrestling with its definition scope to get closure right.


## Vision

Those interested should be able to read the syntax almost immediately after
going through the basics; hopefully, one afternoon is enough to
get into the language. Implementation details
are also automated by the virtual machine, and you only need to
focus on writting algorithms. Finally, the language addresses the most common
development needs with a few simple interfaces.

<br>

At heart, I want to create a comprehensive language that is easy to pick up
and fun to work with for small and mid-sized projects. I hope it's to your liking. 😊

## Opinionated

Blombly is intentionally opinionated on a several principles. Some of these
optinions ended up stronger than mine, with the goal of creating a consistent
way of reading and writting code. My mantra is that having fewer options to
solve the same problems makes it easy to settle in a "proper" implementation
and I am trying to cull any such duplication - unless it saves a lot of boilerplate
like with `in` and `catch` that could both be implemented with some long expression
using `as`.
So, if you think that the language offers two ways to do the same thing, take 
a moment to reflect on what the difference might be; these might be substantial.

<br>

For example, if you want to have a counter that you will increase from different
function calls, the "proper" way to do it is with the following implementation.
Adopting the alternative pattern `counter.value += 1` for some public variable
does not respect the fact that a stable struct state is guaranteed to be preserved
only while struct methods run (atomocity only during member function execution).
Furthermore, the value was made private to prevent
accidentally using the alternative pattern. You could pass the counter as an 
argument though, in which case it becomes a question of whether you want this
argument passing to be a part of your business logic or not.

```java
final counter = new{
    \value = 0;
    inc = {this\value += 1}
    int => value;
}
foo = {counter.inc();}

foo();
foo();
foo();
print(counter|int); // 3
```


## Promises

In the spirit of guiding Blombly's evolution, there are a couple of points 
that I promise to respect. 
I do not consider any of these legally binding,
so you will have to take me at my word that I will stick to them to the
best of my ability, and according to my intent to make a good language.

### 1. Implementation details will never be added. 

If non-algorithmic decisions need to
be made, "good enough" options and automation will be selected. 
If an existing feature's design crystalizes into something that includes
implementation details, those will be removed until only the algorithmic
part remains.

!!! warning
    The only exception to this rule will be numeric vectors 
    - simply because the gains are too great to ignore.

### 2. Safety and running stability have top priority. 

Any feature that is
found to lead to undefined or unsafe behavior will be promptly patched,
regardless of what might depend on it. That is, write Blombly according to
its specification. Request for fixes or certain behavior in GitHub.
Moreover, anyone executing a Blombly program controls
how it affects their machine. If new attack surfaces get discovered,
I will be adding more restrictions to the virtual machine and your
users/main files may need to add additional permissions to re-enable 
certain behavior.

### 3. Backwards compatibility is important.

Though it comes next in order of importance The syntax of version 2.0.0
(expected to be reached withing 2025) will remain stable.
Barring unforeseen events, the syntax of version 3.0.0 (expected
to be reached at late 2026) will remain backwards compatible
barring specifically safety issues. 

<br>

**Compiled bbvm programs are always 
forward compatibe.** Furthermore, there will be very few new language 
features other than support for
more types of IO. There is a chance that some features 
will be deprecated aefore 3.0.0 if I find ways to merge them
with other syntax while maintaining expressiveness.

### 4. The standard library (`libs/`) will be boilerplate.

Ιt will always be simple to implement similar functionality yourself. 
So newcommers will never need to learn the standard library to get
down to coding, and not miss out on anything important.

!!! info
    Although part of the standard library, this promise does not carry
    over to shipped macros like  `in` `default` `=>` `assert`.
    These are not part of the main language only because I am trying to 
    move as much stuff I can outside the virtual machine to give people 
    the option to tinker with the language itself if they want to.
    Such macros are covered extensively in the user guide and can be basically
    considered part of the language. 
