bb.html = new{}

bb.html.html(title, body) = {
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

bb.html.dom(name) = {
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

bb.html.class(str cssclass) => new {
    call(dom) = {
        if(dom.cssclass|len|bool) dom.cssclass += " ";
        dom.cssclass += this..cssclass;
        return dom;
    }
    lt(dom) = {return this.call(dom)}
}

bb.html.css(str style) => new{
    call(dom) = {
        if(dom.style|len|bool) dom.style += ";";
        dom.style += this..style;
        return dom;
    }
    lt(dom) = {return this.call(dom)}
}


bb.html.tr = "tr";
bb.html.td = "td";
bb.html.div = "div";
bb.html.p = "p";
bb.html.h1 = "h1";
bb.html.h2 = "h2";
bb.html.h3 = "h3";
bb.html.h4 = "h4";
bb.html.h5 = "h5";
bb.html.table = "table";
bb.html.tbody = "tbody";
