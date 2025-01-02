# Blombly

A simple programming language for creating reusable dynamic interfaces.

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)



<img src="blombly.png" alt="Logo" width="256"> 


### 🚩 Reusable

Blombly is _behaviorizeable_, which means that modular interfaces written in it have [learnable usage rules](https://www.sciencedirect.com/science/article/pii/S2352220821000778). This way, code exhibits actual usage patterns instead of promoting recipe copy-ing.

### 🚀 Expressive

Focus on writing algorithms. These are interpreted by the **BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM), which automates details like parallelization. The standard library includes operating system and web tools.

### 🦆 Dynamic

Take advantage of features like duck typing, automatic reference counting, and changing which code blocks are inlined. Use `try` to intercept both errors and returns. Blombly syntax is opinionated on not allowing reflection; use [typeless struct](material/typeless.md) principles instead by testing on-the-fly and catching errors.

### 💡 Simple

There are only a few core commands; in addition to declaring and calling code blocks, it is sufficient to know about
`final` `default` `as` `:` `|` `new` `.` `this` `if` `while` `iter` `try` `catch` `fail` `return` `!import`. There is no duplicate functionality aside from the symbols `=>` `!defer` `in` that massively simplify code.