# Blombly

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)

There are many programming languages for writing dynamic yet safe code. You might already have a favorite one! However, safety often reduces expressiveness or relies on non-infallible programmers. Blombly addresses these issues with a small instruction set that is easy to learn and use. It also focuses on programming logic by automating details of how algorithms execute, like parallelization. 

Blombly compiles into an intermediate representation that is interpreted by the **BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM). This replaces jump-based statements to calling code blocks and supports dynamic code inlining as a major language feature. Control flow is managed via conditions, loops, inlining, method calling, return statements for results, and errors on failure.

So, who is Blombly for? It might be a good choice if you are looking for a language that is:


## 🚀 Expressive

Focus on writing algorithms, and leave details like parallelization to the virtual machine.

## 🦆 Dynamic

Take advantage of features like duck typing, automatic garbage collection, and changing which code blocks are inlined. Use `try` to intercept and handle algorithm results, namely errors and returns.

## 💻 Safe

Write memory-safe code with atomic resource acquisition that nonetheless never hangs.

## 💡Simple

The language explicitly hardcodes a preferred alternative when there are multiples. For example, there is only one loop structure (`while`) but other basic functionality lets it iterate over list elements or break prematurely.