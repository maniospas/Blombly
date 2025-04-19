!with colors:
final json = new {
    final NULL = new {str() => "NULL"}
    final \validate(str arg) = {
        if(arg[0]=="[") return arg;
        if(arg[0]=="{") return arg;
        if(arg[0]=="\"") return arg;
        if(arg[0]=="-") return arg;
        if(arg=="NULL") return "\""+arg+"\"";
        if(arg[0]>="0" and arg[0]<="9") return arg;
        fail("This value has not been converted to json: "+arg);
    }
    final list = {
        first = args|next;
        if(first|len==1) if(largs as first|list|iter) first = largs|next else largs = args;
        ret = this\validate(first);
        while(arg as largs|next) ret += ","+this\validate(arg);
        return "["+ret+"]";
    }
    final map = {
        first = args|next;
        if(margs as first|map|iter) first = margs|next else margs = args;
        final unpack(list arg) => "\""+str(arg[0])+"\": "+this\validate(arg[1]);
        ret = unpack(first);
        while(arg as margs|next) ret += ","+unpack(arg);
        return "{"+ret+"}";
    }
    final string(str value) => "\""+value+"\"";
    final parse(text) = {
        final text |= str;
        final state = new{
            n = len(text);
            i = 0;
            current() => text[this.i];
            consume() = {this.i += 1}
        }
        final parse_value() = {
            skip_whitespace();
            if(match("null")) return this.NULL;
            if(match("true")) return true;
            if(match("false")) return false;
            c = state.current();
            if(c=="\"") return parse_string();
            if(c=="{") return parse_object();
            if(c=="[") return parse_array();
            if(c=="-") return parse_number();
            if(c>="0" and c<="9") return parse_number();
            fail("Unexpected character: \\!{c}");
        }
        final parse_object() = {
            state.consume();
            obj = map();
            skip_whitespace();
            if(state.current()=="}") {
                state.consume();
                return obj;
            }
            while(true) {
                skip_whitespace();
                assert state.current()=="\"";
                key = parse_string();
                skip_whitespace();
                assert state.current()==":";
                state.consume();
                skip_whitespace();
                value = parse_value();
                obj[key] = value;
                skip_whitespace();
                c = state.current();
                if(c=="}") {
                    state.consume();
                    return obj;
                }
                assert c==",";
                state.consume();
            }
        }
        final parse_array() = {
            state.consume();
            arr = list();
            skip_whitespace();
            if(state.current()=="]") {
                state.consume();
                return arr;
            }
            while(true) {
                skip_whitespace();
                arr << parse_value();
                skip_whitespace();
                c = state.current();
                if(c=="]") {
                    state.consume();
                    return arr;
                }
                assert c==",";
                state.consume();
            }
        }
        final parse_number() = {
            is_float = false;
            consume_digits = {
                while(state.current()|is_digit) {
                    num_str += state.current();
                    state.consume();
                }
            }
            num_str = "";
            if(state.current()=="-") {
                num_str += state.current();
                state.consume();
            }
            consume_digits:
            if(state.current()==".") {
                num_str += state.current();
                state.consume();
                consume_digits:
                is_float = true;
            }
            if(state.current()=="e" or state.current()=="E") {
                num_str += state.current();
                state.consume();
                is_float = true;
                if(state.current()=="+" or state.current()=="-") {
                    num_str += state.current();
                    state.consume();
                }
                consume_digits:
            }
            if(is_float) return float(num_str);
            return int(num_str);
        }
        final parse_string() = {
            state.consume();
            result = "";
            while(true) {
                c = state.current();
                if(c=="\"") {
                    state.consume();
                    return result;
                }
                if(c=="\\") {
                    state.consume();
                    c = state.current();
                    if(c=="\"") result += c
                    else if(c=="\\") result += "\\"
                    else if(c=="/") result += "/"
                    else if(c=="b") result += "\b"
                    else if(c=="f") result += "\f"
                    else if(c=="n") result += "\n"
                    else if(c=="r") result += "\r"
                    else if(c=="t") result += "\t"
                    // TODO: add support for \uXXXX here
                    else fail("Invalid escape sequence: \\!{c}");
                }
                else result += c;
                state.consume();
            }
        }
        final is_digit(c) = {
            if(c<"0") return false;
            if(c>"9") return false;
            return true;
        }
        final match(query) = {
            n = len(query);
            i = state.i;
            if(i+n>state.n) return false;
            if(text[range(i, i+n)]==query) {
                state.i += n;
                return true;
            }
            return false;
        }
        final skip_whitespace = {
            while(state.i<state.n) {
                c = state.current();
                if(c==" " or c=="\t" or c=="\n" or c=="\r") state.consume() else return;
            }
        }

        value = parse_value();
        skip_whitespace();
        if(state.i!=state.n) fail("Extra data after json end");
        return value;
    }
}