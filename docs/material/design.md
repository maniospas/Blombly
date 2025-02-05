# Language Design

Here is a summary of Blombly's design and philosophy going forward.
The main point is that the language should stay simple, with emphasis on
reusing code without wrestling with its definition scope to get closure right.


## Vision

You should be able to read the syntax almost immediately after
going through the basics. One afternoon worth of reading
is enough to start writting code. Implementation details
are also automated by the virtual machine, and you only need to
focus on writting algorithms. Finally, the language addresses common
development needs with a few simple interfaces.
At heart, I want to create a comprehensive language that is easy to pick up
and fun to work with for small and mid-sized projects. I hope it's to your liking. 😊

<br>

Blombly will always be heavily opinionated on certain aspects of development. 
This a good way to maintain a consistent
style in a broad range of solutions; my mantra is that having fewer but easier to find options 
prevents the creation of divergent ecosystems within the language.
Conversely, I am trying to cull code duplication unless it saves a lot of boilerplate.
Furthermore, the standard library is also an assistant to newer coders by providing known
solutions, but not needed to properly work with the language for those happy
to implement their own solutions.


!!! info
    Although implemented in the standard library, the macros
    `in` `default` `=>` `assert` `catch` need to be understood to get coding.
    These save a huge amount of boilerplate and are treated as part of the language
    and covered extensively in the user guide.
    They are implemented within
    the standard library only because I am trying to 
    move as much stuff I can outside the virtual machine to allow
    tinkering with the language.


## Promises

In the spirit of guiding Blombly's evolution, I have selected a couple of points 
that I promise to respect. I do not consider any of these legally binding,
so you will at best have to take me at my word that I will enforce them to the
best of my ability, and according to my intent to make a good language.

<br>
<span style="font-size:1.2em;color:#00ABAB;">1. Implementation details will never be added.</span>

If non-algorithmic decisions need to
be made, "good enough" options and automation will be selected. 
If an existing feature's design crystalizes into something that includes
implementation details, those will be removed until only the algorithmic
part remains.
The only exception to this rule will be numeric vectors 
- the gains are too great to ignore there (unless proper 
optimizations are found).

<br>

<span style="font-size:1.2em;color:#00ABAB;">2. Safety and running stability have top priority.</span>

Any feature that is
found to lead to undefined or unsafe behavior will be promptly patched,
regardless of what might depend on it. That is, write Blombly according to
its specification. Request for fixes or certain behavior in GitHub.
Moreover, anyone executing a Blombly program controls
how it affects their machine. If new attack surfaces get discovered,
I will be adding more restrictions to the virtual machine and your
users/main files may need to add additional permissions to re-enable 
certain behavior.

<br>

<span style="font-size:1.2em;color:#00ABAB;">3. Backwards compatibility is important.</span>

The language will for the most part remain backwards compatible,
though this comes after safety and stability in terms of priority.
The syntax of version 2.0.0
(expected to be reached within 2025) will remain stable.
Barring unforeseen events, the syntax of version 3.0.0 (expected
to be reached at late 2026) will remain backwards compatible
barring specifically safety issues. 
**Compiled bbvm programs will always remain
forward compatibe.** Furthermore, there will be very few new language 
features other than support for
more types of IO. There is a chance that some features 
will be deprecated aefore 3.0.0 if I find ways to merge them
with other syntax while maintaining expressiveness.

<br>

<span style="font-size:1.2em;color:#00ABAB;">4. The standard library (`libs/`) will be boilerplate.</span>

You don't need to learn it.
Ιt will always be simple to implement similar functionality yourself. 
So if you are already experienced and prefer to implement simple interfaces yourself,
you will not miss out on anything important.

