x = list();
y = list();
while(i in range(100)){
    x << i*0.01;
    y << ((i-50)*0.01)^2;
}
canvas = new{bb.sci.Plot:}
canvas = canvas.plot(x, y :: color=255,0,0,255; title="y=(x-0.5)^2");
canvas = canvas.plot(x, x :: color=0,255,0,255; title="y=x");
canvas.show(width=800; height=600);
