!access ""

g = graphics("MyApp", 800, 600);

logo = new {
    x = 400-50;
    y = 300-50;
    speedx = 0;
    speedy = 0;
    angle = 0;
    update(dt) = {
        this.x += this.speedx*dt*100;
        this.y += this.speedy*dt*100;
        this.angle += 100*dt;
    }
    texture => "docs/blombly.png",this.x,this.y,100,100,this.angle;
}
logox = 400-50;
logoy = 300-50;
logoangle = 0;
font = "playground/fonts/OpenSans-VariableFont_wdth,wght.ttf";

invfps = 1/60;
previous_frame = time();
while(events as g|pop) {
    // draw graphics
    g|clear;
    g << logo.texture();

    // show fps
    fps = (1/invfps)|int|str+" fps";
    g << fps,font,12,700,500,0;

    // process events
    while(event in events) if(event.type=="key::down") {
        if(event.key=="W") logo.speedy -= 1;
        if(event.key=="S") logo.speedy += 1;
        if(event.key=="A") logo.speedx -= 1;
        if(event.key=="D") logo.speedx += 1;
    }

    // update loop
    try logo.update(dt);
    
    // update fps
    current_frame = time();
    dt = current_frame-previous_frame;
    previous_frame = current_frame;
    invfps = invfps*0.99+0.01*dt;
}