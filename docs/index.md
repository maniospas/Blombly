<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blombly</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            flex-direction: column;
            height: 100vh;
            margin: 0;
        }
        .tabs {
            padding: 3px;
            display: flex;
            flex-direction: column;
            border-right: 2px solid #ddd;
        }
        .tab {
            padding: 3px;
            cursor: pointer;
            border-bottom: 1px solid #ddd;
            transition: background 0.3s;
        }
        .tab:hover, .tab.active {
            background: #ddd;
        }
        .content {
            flex-grow: 1;
            padding: 20px;
            padding-top:0px;
            overflow: auto;
        }
        .tab-content {
            display: none;
        }
        .tab-content.active {
            display: block;
        }
    </style>
</head>
<body>
    <h1 style="margin-bottom:0px;">Blombly</h1>
    <p>A simple programming language for reusable dynamic interfaces. Quickly produce sufficiently good solutions for many tasks.</p>
    <p>
        <a href="setup.md">Get started</a> | 
        <a href="https://github.com/maniospas/Blombly/releases/latest">Download</a> | 
        <a href="https://github.com/maniospas/Blombly">GitHub</a>
    </p>

    <br>
    <b> ♻️ Reusable.</b>
    Create <a href="https://www.sciencedirect.com/science/article/pii/S2352220821000778">humanly learnable</a> apis. Functions adapt to where they run.
    <br>
    <b>🚀 Expressive.</b> 
    Focus on writing algorithms with automated parallelism. Blombly ships with operating system and web tools.
    <br>
    <b>🦆 Dynamic.</b> Leverage features like duck typing and dynamic inlining. Syntax is opinionated to prevent multiple solutions.
    <br>
    <b> 💡 Simple.</b> There are only a few core commands; learn Blombly in an afternoon.

    <h2>What it looks like</h2>
    
    <div style="display: flex;">
        <div class="tabs">
            <div class="tab active" onclick="showTab('simple')">Lists</div>
            <div class="tab" onclick="showTab('adapt')">Functions</div>
            <div class="tab" onclick="showTab('inline')">Inlining</div>
            <div class="tab" onclick="showTab('filesystem')">Resources</div>
            <div class="tab" onclick="showTab('graphics')">Graphics</div>
            <div class="tab" onclick="showTab('science')">Science</div>
        </div>
        <div class="content">

            <div id="simple" class="tab-content active">
                
                ```java
                A = 1,2,3,4;
                A << 5; // push
                sum = 0;
                while(i in A) sum += i;
                print("First: !{A[0]}");
                print("Sum: !{sum}");
                print("Len: !{A|len}");
                ```

                <small>Find common operations. Control flow is imperative,
                but | passes values through functions of one argument.</small>
            </div>
            <div id="adapt" class="tab-content">

                ```java
                adder(x, y) = {
                    default xscale = 1;
                    default yscale = 1;
                    return x*xscale + y*yscale;
                }
                add = adder;
                final xscale = 10;
                print(add(1,2));
                print(add(1,2 :: xscale=2;yscale=2));
                ```

                <small>Create functions as runnable code blocks.
                These do not capture state because only structs are allowed to.
                Final variables are immutable and visible 
                from functions running in each scope. 
                Start a new scope after :: and it is transferred to the
                running function.</small>
            </div>
            <div id="inline" class="tab-content">

                ```java
                Point = {
                    norm() => (this.x^2+this.y^2)^0.5;
                    str() => "(!{this.x},!{this.y})";
                }
                p = new{Point:x=1;y=2}
                print(p.norm());
                print(p);
                ```

                <small>Inline code with : to make complicate functions
                or create typeless objects. 
                The symbol => is equivalent to directly returning. 
                Overload operations like addition and string conversion.
                </small>
            </div>
            <div id="graphics" class="tab-content">
                ```java
                g = graphics("MyApp", 800, 600);
                logo = new{
                    x = 350;
                    y = 250;
                    angle = 0;
                    texture() => "docs/blombly.png",this.x,this.y,100,100,this.angle;
                }

                dt = 0;
                prev_t = time();
                while(events as g|pop) {
                    while(event in events) if(event.graphics::type=="key::down") {
                        if(event.graphics::key=="A") logo.angle -= 360*dt;
                        if(event.graphics::key=="D") logo.angle += 360*dt;
                    }
                    g << logo.texture();
                    dt = time()-prev_t;
                    prev_t = time();
                }
                ```
            </div>
            <div id="filesystem" class="tab-content">
            
                ```java
                !access "https://" 
                // use some help from the standard library
                response = "https://www.google.com"|file|bb.string.join("\n"); 
                print("Len: !{response|len}");
                ```

                <small>Access and modify perfomissions must be declared in the main file;
                the correspond to URI prefixes.
                The file system, a virtual file system, and web resources have the same interface.
                </small>
            </div>

            <div id="science" class="tab-content">
            
                ```java
                x = list();
                while(i in range(100)) x << i*0.01;
                x |= vector; // shorthand for x = vector(x);
                y = (x-0.5)^2;

                canvas = new{bb.sci.Plot:} // standard library utility
                canvas.plot(x, y :: color=255,0,0,255; title="y=(x-0.5)^2");
                canvas.plot(x, x :: color=0,255,0,255; title="y=x");
                canvas.show(width=800; height=600);

                ```
            </div>
        </div>
    </div>

    <script>
        function showTab(tabId) {
            document.querySelectorAll('.tab-content').forEach(tab => {
                tab.classList.remove('active');
            });
            document.getElementById(tabId).classList.add('active');
            document.querySelectorAll('.tab').forEach(tab => {
                tab.classList.remove('active');
            });
            event.currentTarget.classList.add('active');
        }
    </script>
</body>

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

</html>
