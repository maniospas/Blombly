final html = new{}

html.html(title, body) = {
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

html.dom(name) = {
    default style = "";
    return new {
        uses dom;
        name |= str;
        style = "";
        cssclass = "";
        contents = list();
        \str = {
            // combine contents
            contents = "";
            while(element in this.contents)
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
            child = name|str|dom;
            push(this.contents, child);
            return child;
        }
        \lt(other) = {
            push(this.contents, other);
            return this;
        }
        \div(other) = {return this.child(other)}
    }
}

html.class(str cssclass) => new {
    call(dom) = {
        if(dom.cssclass|len|bool) dom.cssclass += " ";
        dom.cssclass += this..cssclass;
        return dom;
    }
    lt(dom) = {return this.call(dom)}
}

html.css(str style) => new{
    call(dom) = {
        if(dom.style|len|bool) dom.style += ";";
        dom.style += this..style;
        return dom;
    }
    lt(dom) = {return this.call(dom)}
}


html.tr = "tr";
html.td = "td";
html.div = "div";
html.p = "p";
html.h1 = "h1";
html.h2 = "h2";
html.h3 = "h3";
html.h4 = "h4";
html.h5 = "h5";
html.table = "table";
html.tbody = "tbody";
