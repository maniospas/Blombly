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

    <div>
        <div class="tabs">
            <div class="tab active" onclick="showTab('simple')">Lists</div>
            <div class="tab" onclick="showTab('adapt')">Functions</div>
            <div class="tab" onclick="showTab('inline')">Inlining</div>
            <div class="tab" onclick="showTab('filesystem')">Resources</div>
            <div class="tab" onclick="showTab('graphics')">Graphics</div>
            <div class="tab" onclick="showTab('science')">Science</div>
            <div class="tab" onclick="showTab('utility')">Terminal</div>
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
                    path = "docs/blombly.png";
                    img() => this.path,this.x,this.y,100,100,this.angle;
                }

                dt = 0;
                prev_t = time();
                while(events as g|pop) {
                    // treat :: as one charcater, like an underscore
                    while(e in events) if(e.io::type=="key::down") {
                        if(e.io::key=="A") logo.angle -= 360*dt;
                        if(e.io::key=="D") logo.angle += 360*dt;
                    }
                    g << logo.img();
                    dt = time()-prev_t;
                    prev_t = time();
                }
                ```

            </div>
            <div id="filesystem" class="tab-content">
            
                ```java
                // control permissions
                !access "https://" 
                f = file("https://www.google.com");

                // from the standard library
                contents = f|bb.string.join("\n"); 
                print("Len: !{contents|len}");
                ```

            </div>

            <div id="science" class="tab-content">
            
                ```java
                // apply x = x<<i*0.01 across several i
                x = !gather(list(), <<){while(i in range(100)) yield i*0.01;}
                
                x |= vector; // shorthand for x = vector(x);
                y = (x-0.5)^2;

                canvas = new{bb.sci.Plot:} // from the standard library
                canvas.plot(x, y :: color=255,0,0,255; title="y=(x-0.5)^2");
                canvas.plot(x, x :: color=0,255,0,255; title="y=x");
                canvas.show(width=800; height=600);
                ```
            </div>


            <div id="utility" class="tab-content">
            
                ```bash
                # prints if no semicolons
                ./blombly 'log(3)+1'
                2.098612
                ```

                ```bash
                # directly run code
                ./blombly 'n=10; fmt(x)=>x[".3f"]; print(fmt(2.5^n))'
                9536.743
                ```

                ```bash
                # the standard library is there too
                ./blombly 'bb.string.md5("this is a string")'
                b37e16c620c055cf8207b999e3270e9b
                ```
            </div>
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
