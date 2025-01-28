<h1 style="margin-bottom:0px;">Blombly</h1>

A simple programming language for reusable dynamic interfaces. 

[Get started](setup.md) | [Download](https://github.com/maniospas/Blombly/releases/latest) | [GitHub](https://github.com/maniospas/Blombly)


<br>
<br>
<b> ♻️ Reusable.</b>
Instead of copying recipes, code admits [learnable usage patterns](https://www.sciencedirect.com/science/article/pii/S2352220821000778).
<br>
<b>🚀 Expressive.</b> 
Focus on writing algorithms with automated parallelism. Blombly ships with operating system and web tools.
<br>
<b>🦆 Dynamic.</b> Leverage features like duck typing and dynamic inlining. Syntax is opinionated just enough to prevent multiple solutions.
<br>
<b> 💡 Simple.</b> There are only a few core commands and mostly one way to do things. Learn Blombly in an afternoon.

<br>
<br>

Blombly has a few features that make it unique. First, it 
automatically parallelizes functions while allowing side-effects on structs
and imperative programming in general. Furthermore, closure (visible symbols) consists of the `final`
variables of the execution scope; preserve definition information on callable structs,
but functions adapt to where they run.
Also dynamically inline multiple code blocks *at runtime*, for example to reusing code
when creating typeless structs. Finally, Blombly forgoes reflection to force logical consistency
through error checking.

!!! tip 
    Despite parallel execution, write code similar to what you are used to because: <br>
    a) The state of structs does not change while their methods run. <br>
    b) Functions conclude before their returned values are used, and before their calling scope is exited. <br>


<style>
.md-sidebar {
    display: none;
}

@media screen and (max-width: 76.2344em) {
    .md-sidebar {
        display: block;
    }
}
</style>