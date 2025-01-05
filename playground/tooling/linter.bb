highlight(code) = {
    end_word = {
        try {
            if(word|len|bool==false) 
                return;
            if(in_comment) {
                ret += bb.ansi.lightgreen+word+bb.ansi.reset;
                return;
            }
            if(in_string) {
                ret += bb.ansi.green+word+bb.ansi.reset;
                return;
            }
            if(word=="as" or word=="in" or word=="push" or word=="pop" or word=="next" or word=="clear") {
                ret += bb.ansi.cyan+word+bb.ansi.reset;
                return;
            }
            if(word[0]=="!") {
                ret += bb.ansi.yellow+word+bb.ansi.reset;
                return;
            }
            if(word[0]=="@") {
                ret += bb.ansi.yellow+word+bb.ansi.reset;
                return;
            }
            if(word=="new" or word=="if" or word=="while" or word=="catch" or word=="return" or word=="else" or word=="bbvm" or word=="assert" or word=="defer" or word=="default" or word=="try" or word=="fail") {
                ret += bb.ansi.lightpurple+word+bb.ansi.reset;
                return;
            }
            if(word=="len" 
                or word=="bool" 
                or word=="int" 
                or word=="final" 
                or word=="float" 
                or word=="call" 
                or word=="server" 
                or word=="str" 
                or word=="server" 
                or word=="print" 
                or word=="file" 
                or word=="iter" 
                or word=="this" 
                or word=="false" 
                or word=="add" 
                or word=="sub" 
                or word=="mul" 
                or word=="div"
                or word=="mod"
                or word=="pow"
                or word=="or"
                or word=="and"
                or word=="not"
                or word=="time"
                or word=="random"   
                or word=="bool" 
                or word=="true") {
                ret += bb.ansi.cyan+word+bb.ansi.reset;
                return;
            }
            ret += word; 
        }
        word="";
    }

    ret = bb.ansi.reset;
    word = "";
    pos = 0;
    n = code|len;
    in_string = false;
    in_fstring = false;
    in_comment = false;

    while(pos<n) try {
        c = code[pos];
        pos += 1;
        if(c=="\n" and in_comment) {
            end_word:
            ret += c;
            in_comment = false;
            return
        }
        if(c=="/" and pos<n) if(code[pos]=="/") in_comment = true;
        if(in_comment) {
            word += c;
            return;
        }
        if(c=="\"") {
            if(in_string) {
                word += c;
                end_word:
                in_string = false;
                return;
            }
            end_word:
            in_string = true;
        }
        if(in_string or in_fstring) {
            if(in_fstring and c=="\}") {
                end_word:
                ret += bb.ansi.cyan+c+bb.ansi.reset;
                in_string = true;
                in_fstring = false;
                return;
            }
            if(not in_fstring and c=="\{") {
                end_word:
                ret += bb.ansi.cyan+c+bb.ansi.reset;
                in_fstring = true;
                in_string = false;
                return;
            }
            if(not in_fstring) {
                word += c;
                return;
            }
        }
        if(c=="=" and pos<n) if(code[pos]==">") {
            end_word:
            ret += bb.ansi.lightpurple+"=>"+bb.ansi.reset;
            pos += 1;
            return;
        }
        if(c=="-" and pos<n) if(code[pos]==">") {
            end_word:
            ret += bb.ansi.lightpurple+"->"+bb.ansi.reset;
            pos += 1;
            return;
        }
        if(c==":") {
            end_word:
            ret += bb.ansi.cyan+c+bb.ansi.reset;
            return;
        }
        if(c=="\{" or c=="\}") {
            end_word:
            ret += bb.ansi.cyan+c+bb.ansi.reset;
            return;
        }
        if(c==">" or c=="<" or c=="+" or c=="-" or c=="*" or c=="^" or c=="%" or c=="|" or c=="." or c=="=" or c=="/" or c=="[" or c=="]" or c=="(" or c==")" or c==";" or c==",") {
            end_word:
            ret += bb.ansi.reset+c+bb.ansi.reset;
            return;
        }
        if(c==" " or c=="\n") {
            end_word:
            ret += c;
            return;
        }
        word += c;
    }
    end_word:
    return ret+bb.ansi.reset;
}


path = "File for code highlighting:"|read;
print("");
print("====================================================");
code = bb.files.read(path);
print(code|highlight);
print("====================================================");