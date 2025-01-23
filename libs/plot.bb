sci = new {
    Plot = {
        final plots = list();
        defer {
            default axes = 255,255,255,255;
            default shade = 255,255,255,32;
            default text = 255,255,255,255;   
        }

        final plot(vector x, vector y) = {
            default color = this.axes;
            default title = "";
            this.plots << new {x=x;y=y;color=color;title=title}
            return this;
        }

        final show() = {
            default width = 400;
            default height = 200;
            default title = "Blombly plot";
            default font = "playground/fonts/OpenSans-VariableFont_wdth,wght.ttf";

            canvas = graphics(title, width, height);
            px = 25;
            py = 25;
            width -= 50;
            height -= 50;

            while(plot in this.plots) {
                default minx = inf  else minx = min(minx, plot.x|min);
                default maxx = -inf else maxx = max(maxx, plot.x|max); 
                default miny = inf  else miny = min(miny, plot.y|min);
                default maxy = -inf else maxy = max(maxy, plot.y|max);
            }

            while(events as canvas|pop) {
                // draw all plots
                while(plot in this.plots) {
                    plot:
                    nx = (x-minx)/(maxx-minx);
                    ny = (y-miny)/(maxy-miny);
                    canvas << color;
                    while(i in range(x|len-1)) 
                        canvas << "line",
                                  nx[i]*width+px,
                                  height-ny[i]*height+py,
                                  nx[i+1]*width+px,
                                  height-ny[i+1]*height+py;
                }

                // draw axes
                canvas << this.axes;
                canvas << "line",px,py+height,px+width,py+height;
                canvas << "line",px,py,px,py+height;

                // count legend elements
                countplots = 0;
                while(plot in this.plots) if(plot.title|len!=0) countplots += 1;
                if(countplots==0) return;

                // draw legend box
                canvas << this.shade;
                canvas << "rect",px+width-200,py,px+width,py+18*countplots+8;
                canvas << this.axes;
                canvas << "orect",px+width-200,py,px+width,py+18*countplots+8;

                // draw plot legends
                i = 0;
                while(plot in this.plots) try {
                    if(plot.title|len==0) return;
                    tx = px+width-200;
                    ty = py+18*i+4;
                    canvas << plot.color;
                    canvas << "line",tx+4,ty+8,tx+20,ty+8;
                    canvas << this.text;
                    canvas << plot.title,font,14,tx+30,ty,0;
                    i += 1;
                }
                
            }
        }
    } // Plot
} // sci

