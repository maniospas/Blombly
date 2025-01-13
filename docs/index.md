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

Blombly is opinionated on several characteristics that set it apart from other languages. These change slightly
how code is written compared to most other languages. The next tip summarizes the cumulative effect of various
features on reasoning about your code:

!!! tip
    You can reason about struct state only while its methods run;
    consider all non-final data and called methods of other structs 
    to be volatile.


In detail, closure (visible symbols) comprises the final values of the scope
where functions and struct methods are executed. There exists syntax to maintain values 
encountered during definition, but in most scenarios functions adapt to where they run.
Finally, multiple code blocks are dynamically selected
and inlined, for example struct creation, to create complex combinations. 
Thus, structs are typeless and Blombly does not use reflection to enforce logical
consistency checking through a combination of duck typing errors, 
primitive type conversions errors, and assertions.

<br>

Most importantly, function and method calls are treated as potentially concurrent if they do
not use each other's outputs. Blombly automatically parallelizes them at runtime
with a scheduler that never deadlocks, but they might have side effects. 
To reason about those in certain situations, struct methods that could end up calling each other 
run in the same thread without concurrent modifications.


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