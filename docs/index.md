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
            flex-direction: row;
            border-bottom: 2px solid #ddd;
            flex-wrap: wrap; 
        }
        .tab {
            padding: 3px;
            padding-right: 7px;
            padding-left: 7px;
            cursor: pointer;
            transition: background 0.3s;
        }
        .tab:hover, .tab.active {
            background: #ccc;
            color: black;
        }
        .content {
            flex-grow: 1;
            padding: 20px;
            padding-top:0px;
            overflow: auto;
            width: 100%;
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
        <a href="https://blombly.readthedocs.io/en/latest/setup">Get started</a> | 
        <a href="https://github.com/maniospas/Blombly/releases/latest">Download</a> | 
        <a href="https://github.com/maniospas/Blombly">GitHub</a>
    </p>

    <br>
    <b> ♻️ Reusable.</b>
    Create <a href="https://www.sciencedirect.com/science/article/pii/S2352220821000778">humanly learnable</a> apis. Functions adapt to the scope where they run.
    <br>
    <b>🚀 Expressive.</b> 
    Focus on writing algorithms and leave implementation details to the virtual machine. Blombly ships with operating system, graphics, and web tools.
    <br>
    <b>🦆 Dynamic.</b> Leverage features like duck typing and dynamic inlining.
    <br>
    <b> 💡 Simple.</b> There are only a few core commands; learn Blombly in an afternoon.

    <h2>What it looks like</h2>
    
    <div>
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

                // | calls functions of one argument
                print("Len: !{A|len}");
                ```

            </div>
            <div id="adapt" class="tab-content">

                ```java
                adder(x, y) = {
                    default xscale = 1;
                    default yscale = 1;
                    return x*xscale + y*yscale;
                }
                add = adder; // code as value

                // acess finals of running scope
                final xscale = 10;
                print(add(1,2));

                // move values after :: to the function
                print(add(1,2 :: xscale=2;yscale=2));
                ```

            </div>
            <div id="inline" class="tab-content">

                ```java
                Point = {
                    // => translates to = {return ...}
                    norm() => (this.x^2+this.y^2)^0.5;
                    str() => "(!{this.x},!{this.y})";
                }

                // inline code with :
                p = new{Point:x=1;y=2}
                print(p.norm());

                // overloaded str operation
                print(p);
                ```
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
                    while(event in events) if(event.io::type=="key::down") {
                        if(event.io::key=="A") logo.angle -= 360*dt;
                        if(event.io::key=="D") logo.angle += 360*dt;
                    }
                    g << logo.texture();
                    dt = time()-prev_t;
                    prev_t = time();
                }
                ```

            </div>
            <div id="filesystem" class="tab-content">
            
                ```java
                // set access permissions
                !access "https://" 

                path = "https://www.google.com";
                contents = path|file|bb.string.join("\n"); 
                print("Len: !{contents|len}");
                ```

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
