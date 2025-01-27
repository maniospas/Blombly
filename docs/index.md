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
<b>🦆 Dynamic.</b> Leverage features like duck typing and dynamic inlining. Syntax is opinionated in favor of [typeless oop](material/typeless.md).
<br>
<b> 💡 Simple.</b> There are only a few core commands.

<br>
<br>

Blombly has a few features that set it apart from other languages. First, it 
automatically parallelizes functions while allowing you to create side-effects on structs (aka objects).
Most parallel languages do no offer this option and therefore are not very good at
object-oriented or in general imperative programming.
Furthermore, closure (visible symbols) contains the `final`
variables of the execution scope; you can preserve definition information on callable structs,
but usually functions adapt to where they run.
You can also dynamically inline multiple code blocks *at runtime*. This is a key aspect of reusing code
when creating typeless structs. Finally, Blombly does not use reflection to enforce logical
consistency checking through a combination of duck typing errors, 
primitive type conversions errors, and assertions.

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