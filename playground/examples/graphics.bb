!access ""
g = graphics("MyApp", 800, 600);

logo = new {
    size = 200;
    x = 400-size/2;
    y = 300-size/2;
    speedx = 0;
    speedy = 0;
    angle = 0;
    update(dt) = {
        this.x += this.speedx*dt*100;
        this.y += this.speedy*dt*100;
        this.angle += 100*dt;
        return this;
    }
    texture() => "docs/blombly.png",this.x,this.y,this.size,this.size,this.angle;
}

font = "playground/fonts/OpenSans-VariableFont_wdth,wght.ttf";
invfps = 1/60;
previous_frame = time();
while(events as g|pop) {
    // draw graphics
    g << logo.texture();

    // show fps
    fps = (1/invfps)|int|str+" fps";
    g << fps,font,12,800-42,600-20,0;

    // process events
    while(event in events) if(event.graphics::type=="key::down") {
        if(event.graphics::key=="W") logo.speedy -= 1;
        if(event.graphics::key=="S") logo.speedy += 1;
        if(event.graphics::key=="A") logo.speedx -= 1;
        if(event.graphics::key=="D") logo.speedx += 1;
    }
    
    // update fps
    current_frame = time();
    dt = current_frame-previous_frame;
    previous_frame = current_frame;
    invfps = invfps*0.99+0.01*dt;

    // update loop (use try for forceful synchronization)
    logo = logo.update(dt);
}