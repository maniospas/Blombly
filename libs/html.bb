final html::INFO as {
    name    = "html";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Provides functions to help write and manage
    \n simple html dom. This can be converted to
    \n string.
    \n
    ";
}


html(title, body) = {
    default script = "";
    default src = "";
    default stylesheet = "";
    return "<!DOCTYPE html>
        <html>
        <head>
            <title>{title|str}</title>
            <link href='{stylesheet}' rel='stylesheet'>
        </head>
        <body>
            <script src='{src|str}'>{script|str}</script>
            {body}
        </body>
        </html>
    ";
}

dom(name) = {
    default style = "";
    return new {
        dom = dom;
        name |= str;
        style = "";
        cssclass = "";
        contents = list();
        \str = {
            // combine contents
            contents = "";
            while(element as next(!of this.contents|iter))
                contents = contents + element|str;
            // additional element values
            preample = "";
            if(this.cssclass|len|bool) preample += " class='{this.cssclass}'";
            if(this.style|len|bool) preample += " style='{this.style}'";
            // compose everything
            ret = "<{this.name}{preample}>{contents}</{this.name}>";
            return ret;
        }
        child(name) = {
            child = name|(this.dom);
            push(this.contents, child);
            return child;
        }
        \add(other) = {
            push(this.contents, other);
            return this;
        }
    }
}

cssclass(cssclass) = {
    return new {
        cssclass |= str;
        \call(dom) = {
            if(dom.cssclass|len|bool) dom.cssclass += " ";
            dom.cssclass += this.cssclass;
            return dom;
        }
    }
}

style(style) = {
    return new {
        style |= str;
        \call(dom) = {
            if(dom.style|len|bool) dom.style += ";";
            dom.style += this.style;
            return dom;
        }
    }
}
