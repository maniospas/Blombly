#include "libs/def"
#include "libs/loop"
def::simplify;

final docfmt::INFO as {
    name    = "docfmt";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Converts various documentation formats to each other.";
}


struct docfmt {
    use env::hbar;
    fn \lines(text) {
        var it = iter(text|str);
        lines = list();
        current = "";
        while(c as bbv::next(it)) try {
            if(c=="\n") {
                push(lines, current);
                current = "";
                return;
            }
            current = current + c;
        }
        if(len(current)!=0)
            push(lines, current);
        return lines;
    }

    fn rst2md(text) {
        lines = \lines(text);
        i = 0;
        while(i<len(lines)) {
            line = lines[i];
            if(len(line)>3) {
                if(line[range(2)]=="--") {
                    if(i<len(lines)-1)
                        lines[i+1] = "# "+lines[i+1];
                    lines[i] = "";
                }
                if((line[range(2)]==" -")and(i>0)) {
                    lines[i-1] = "## "+lines[i-1];
                    lines[i] = "";
                }
            }
            i = i+1;
        }
        result = "";
        it = iter(lines);
        while(line as bbv::next(lines))
            result = result + line + "\n";
        return result;
    }
}