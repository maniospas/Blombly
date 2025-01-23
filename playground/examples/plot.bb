plot(vector x, vector y) = {
    default width = 400;
    default height = 200;
    canvas = graphics("plot", width, height);
    nx = (x-min(x))/(max(x)-min(x));
    ny = (y-min(y))/(max(y)-min(y));
    px = 25;
    py = 25;
    width -= 50;
    height -= 50;
    font = "playground/fonts/OpenSans-VariableFont_wdth,wght.ttf";
    while(events as canvas|pop) {
        // draw lines
        n = x|len;
        assert n>=2;
        assert y|len == n;
        canvas << 255,0,0,255;
        while(i in range(n-1)) {
            x1 = nx[i]*width+px;
            y1 = height-ny[i]*height+py;
            x2 = nx[i+1]*width+px;
            y2 = height-ny[i+1]*height+py;
            canvas << "line",x1,y1,x2,y2;
        }

        // draw axes
        canvas << 255,255,255,255;
        canvas << "line",px,py+height,px+width,py+height;
        canvas << "line",px,py,px,py+height;
    }

}


x = list();
y = list();
while(i in range(100)){
    x << i*0.01;
    y << ((i-50)*0.01)^2;
}
plot(x,y :: width=800; height=600);