#include "libs/loop"


final html(title, body) = {
    default script = "";
    return "<!DOCTYPE html>
        <html>
        <head>
            <title>{title|str}</title>
            <script>{script|str}</script>
        </head>
        <body>
        {body}
        </body>
        </html>
    ";
}

final dom(name) = {
    default style = "";
    return new {
        name |= str;
        style = "";
        cssclass = "";
        contents = list();
        \str = {
            // combine contents
            contents = "";
            while(element as loop::next(this.contents))
                contents = contents + element|str;
            // additional element values
            preample = "";
            if(this.cssclass|len|bool) preample += " class='{this.cssclass}'";
            if(this.style|len|bool) preample += " style='{this.style}'";
            // compose everything
            ret = "<{this.name}{preample}>{contents}</{this.name}>";
            return ret;
        }
        \add(other) = {push(this.contents, other);return this}
    }
}

final cssclass(cssclass) = {
    return new {
        cssclass |= str;
        \call(dom) = {
            if(dom.cssclass|len|bool) dom.cssclass += " ";
            dom.cssclass += this.cssclass;
            return dom;
        }
    }
}

final style(style) = {
    return new {
        style |= str;
        \call(dom) = {
            if(dom.style|len|bool) dom.style += ";";
            dom.style += this.style;
            return dom;
        }
    }
}
