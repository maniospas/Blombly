# Language Design


## Vision

Blombly's syntax should be easy to understand almost immediately after 
learning the basics; a single afternoon of reading is enough to start 
writing code. Implementation details are automated by the virtual machine, 
allowing you to focus solely on implementing algorithms with components
that satisfy common development needs.
At heart, I want to create a comprehensive language that is easy to pick up
and fun to work with for small and mid-sized projects. I hope it's to your liking. 😊

<br>

Blombly is opinionated about certain aspects of development. 
This ensures a consistent style across a wide range of solutions. 
My philosophy is that having fewer but easier-to-find options prevents 
the fragmentation of the language's ecosystem. Conversely, I aim to eliminate 
code duplication, unless the available alternatives significantly reduce boilerplate.
The standard library is helpful for newer coders by providing well-known algorithms,
but anyone can implement custom solutions.


!!! info
    Although implemented in the standard library, the macros
    `in` `default` `=>` `assert` `catch` are essential for coding.
    These eliminate a lot of boilerplate and are treated as integral parts of the language
    in the user guide. They reside in the standard library because I want to 
    let anyone tinker with high-level features.


## Guidelines

To guide Blombly's evolution, I commit to the following principles. 
These are *not* legally binding,
so you will have to take my word that I will enforce them to the
best of my ability. My intent is to make a good language that emphasizes
simplicity and reusability without wrestling with code modifications.

<br>
<span style="font-size:1.2em;color:#00ABAB;">1. The language will never make you think about implementation details.</span>

Non-algorithmic decisions will always be automated with "good enough" implementations.
If a feature's design exposes details unrelated to problem solving, those details will be removed.
The only exception to this rule is numeric vectors, 
as their performance benefits are too great to ignore.

<br>

<span style="font-size:1.2em;color:#00ABAB;">2. Safety and running stability have top priority.</span>

Any found undefined or unsafe behavior will be promptly patched, regardless of dependencies. 
In other words, Blombly will actively try to conform to its specification. If you encounter issues, 
please request fixes or improved interfaces on GitHub.
Furthermore, anyone running a Blombly program should have full control over how it affects their system. 
If new security vulnerabilities are discovered, I will impose additional restrictions on the virtual machine. 
This could require users or main files to grant additional permissions to run the same programs in
the future.

<br>

<span style="font-size:1.2em;color:#00ABAB;">3. Backwards compatibility is important.</span>

The language will remain largely backwards compatible, though safety and stability take precedence. 
The syntax of version 2.0.0 (expected in 2025) will remain stable. Barring unforeseen circumstances, 
version 3.0.0 (expected in late 2026) will also be backwards compatible, except in cases related to
security.
**Compiled bbvm programs will always remain forward compatible.** Additionally, new language features 
will be minimal and primarily focused on expanding support for different types of I/O. Some features 
may be deprecated before 3.0.0 if they can be merged with other syntax while maintaining expressiveness.

<br>

<span style="font-size:1.2em;color:#00ABAB;">4. The standard library (`libs/`) will be boilerplate.</span>

You do not need to learn the standard library to use Blombly effectively, 
and it will always be simple enough to implement similar functionality yourself.
If you are an experienced programmer who and have an easier time writing custom interfaces
than reusing code, you will not miss out on anything essential.
