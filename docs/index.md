# Blombly

A programming language for reusable interfaces.

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)


There are many programming languages for writing dynamic yet safe code. You might already have a favorite one! However, safety often reduces expressiveness or relies on non-infallible programmers. Blombly addresses these issues with a small instruction set that is easy to learn and use. It also focuses on programming logic by automating details of how algorithms execute, like parallelization. 

So, who is Blombly for? It might be a good choice if you are looking for a language that is:

## 🚩 Reusable

The language is _behaviorizeable_, which means that modular interfaces written in it have [learnable usage rules](https://www.sciencedirect.com/science/article/pii/S2352220821000778). This way, code has learnable usage patterns instead of promoting recipe copy-ing.

## 🚀 Expressive

Focus on writing algorithms. These are compiled into an intermediate representation that is interpreted by the **BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM), which handles details like parallelization on its own. 

_Note: The current implementation is purely interpreted, but a JIT may be added in the future._

## 🦆 Dynamic

Take advantage of features like duck typing, automatic garbage collection, and changing which code blocks are inlined. Use `try` to intercept and handle algorithm results, namely errors and returns.

## 💡Simple

The language hardcodes a preferred alternative when there are multiples. For example, there is only one loop structure (`while`) but other basic functionality lets it iterate over list elements or break prematurely.