<h1 style="margin-bottom:0px;">Blombly</h1>

A simple programming language for reusable dynamic interfaces.

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)

<br>
<b> ♻️ Reusable.</b>
Blombly is _behaviorizeable_, which means that modular interfaces written in it have [learnable usage rules](https://www.sciencedirect.com/science/article/pii/S2352220821000778). This way, code exhibits actual usage patterns instead of promoting recipe copy-ing.

<br>
<b>🚀 Expressive.</b> Focus on writing algorithms. These are interpreted by the **BLO**ck ase**MBLY** **V**irtual **M**achine (blomblyVM), which automates details like parallelization. The standard library includes operating system and web tools.

<br>
<b>🦆 Dynamic.</b> Leverage features like duck typing, automatic reference counting, and dynamic inlining. Use `try` to intercept both errors and returns. Blombly syntax is opinionated on using [typeless structs](material/typeless.md) and testing on-the-fly for errors.

<br>
<b> 💡 Simple.</b> There are only a few core commands; in addition to builtin data types and operations, and declaring and calling code blocks, it suffices to know about the following: `!import`, `!access`, `!modify`, `final`, `default`, `as`, `:`, `|`, `new`, `.`, `this`, `if`, `while`, `iter`, `try`, `catch`, `fail`, `return`, `=>`, `defer`, `in`