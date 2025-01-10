<h1 style="margin-bottom:0px;">Blombly</h1>

A simple programming language for reusable dynamic interfaces.

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)

<br>
<br>
<br>
<b> ♻️ Reusable</b><br>
Blombly code has [learnable usage patterns](https://www.sciencedirect.com/science/article/pii/S2352220821000778) instead of promoting recipe copying.

<br>
<b>🚀 Expressive</b><br>Focus on writing algorithms. These are interpreted by the **BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM), which automates details like parallelization. The standard library includes operating system and web tools.

<br>
<b>🦆 Dynamic</b><br>Leverage features like duck typing, automatic reference counting, and dynamic inlining. Use `try` to intercept both errors and returns. Blombly syntax is opinionated on using [typeless structs](material/typeless.md) and testing on-the-fly for errors.

<br>
<b> 💡 Simple</b><br>There are only a few core commands; in addition to builtin data types and operations, and declaring and calling code blocks, it suffices to know about the following: `!import`, `!access`, `!modify`, `final`, `default`, `as`, `:`, `|`, `new`, `.`, `this`, `if`, `while`, `iter`, `try`, `catch`, `fail`, `return`, `=>`, `defer`, `in`