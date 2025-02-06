g = graphics("MyApp", 800, 600);
logo = new{
    x = 350;
    y = 250;
    angle = 0;
    texture() => "docs/blombly.png",this.x,this.y,100,100,this.angle; // angle is zero
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