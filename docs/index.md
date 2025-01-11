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


## About

Blombly is opinionated on several characteristics that set it appart from other languages. These change slightly
how code is written in complex scenarios.

<br>

Most importantly, function and method calls should be treated as potentially concurrent if they do
not use each other's outputs. Thus, the virtual machine automatically parallelizes them at runtime
with a scheduler that never deadlocks. Many race conditions are removed by making
struct methods that could end up calling each other run in the same thread without concurrent 
modifications.

<br>

Moreover, closure (visible symbols) comprises the final values of the scope
where functions and struct methods are executed. There exists syntax to maintain values 
encountered during definition, but in most scenarios functions adapt to where they run.
Finally, multiple code blocks are dynamically selected
and inlined, for example struct creation, to create complex combinations. 
Thus, structs are typeless and Blombly does not use reflection to enforce logical
consistency checking through a combination of duck typing errors, 
primitive type conversions errors, and assertions.


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