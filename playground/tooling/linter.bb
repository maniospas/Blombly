final elementof(all) => new {
    all = all;
    call(query) = {
        while(symbol in this.all) if(query==symbol) return true;
        return false;
    }
}

final append_word = {
    if(word|elementof(("final","float","int","str","iter","print","read"))) ret += bb.ansi.purple;
    if(word|elementof(("return","if","do","while","catch","new"))) ret += bb.ansi.lightblue;
    if(word|len|bool) ret += word+bb.ansi.reset;
    word = "";
}

final highlight(str code) = {
    final n = len(code);
    word = "";
    ret = "";
    in_string = false;
    while(pos in range(n)) do {
        c = code[pos];
        if(c=="\"") {
            if(in_string) {
                ret += bb.ansi.green+word+c+bb.ansi.reset;
                word = "";
                in_string = false;
                return;
            }
            else {
                append_word:
                word = c;
                in_string = true;
                return;
            }
        }
        if(in_string) {
            word += c;
            return;
        }
        if(c|elementof("{}()!|:")) {
            append_word:
            ret += bb.ansi.yellow+c+bb.ansi.reset;
            return;
        }
        if(c|elementof(" \n;+-/*^=<>%")) {
            append_word:
            ret += c;
            return;
        }
        word += c;
    }
    append_word:

    return ret+bb.ansi.reset;
}


path = "File for code highlighting:"|read;
print("");
print("====================================================");
code = bb.os.read(path);
print(code|highlight);
print("====================================================");