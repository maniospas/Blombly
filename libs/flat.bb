!macro{list::cat(@a)} as {list(@a)}
!macro{list::cat(@a,@b)} as {list::cat(@a)+list::cat(@b)}


!macro{!flat @Point_(@fields);} as {
    !local{!flat @@Other_ (@@begin0 @Point_ @@name_ @@end0);} as {
        !include !comptime do {
            other = !codestring(@@Other_);
            begin = !codestring(@@begin0);
            end = !codestring(@@end0);
            fields = !codestring(@fields)|bb.string.split(",");
            symbol = !codestring(@@name_);
            ret = "";
            while(field in fields) {
                if(ret|len|bool) ret += ",";
                ret += "!{symbol}.!{field}";
            }
            return "!flat "+other+"("+begin+ret+end+");";
        }
    }
    !local{@Point_ @@name_} as {
        !local{@@name_} as {
            !include !comptime do {
                fields = !codestring(@fields)|bb.string.split(",");
                symbol = !codestring(@@name_);
                ret = "";
                while(field in fields) {
                    if(ret|len!=0) ret += ",";
                    ret += "!symbol{!{symbol}.!{field}}";
                }
                return ret;
            }
        }
        !include !comptime do {
            fields = !codestring(@fields)|bb.string.split(",");
            while(i in range(fields|len)) fields[i] = fields[i]|bb.string.split(" ")|bb.string.join("");
            symbol = !codestring(@@name_);
            ret = "";
            while(field in fields) {
                if(ret|len!=0) ret += ",";
                ret += "!symbol(!{symbol}.!{field})";
            }
            while(field in fields) ret += "!local{!{symbol}.!{field}} as {!symbol(!{symbol}.!{field})}\n";
            details = "";
            all = map();
            while(field in fields) {
                split = field|bb.string.split(".");
                if(split|len>1) {
                    parent = split[range(0,split|len-1)]|bb.string.join(".");
                    fields << parent;
                    catch(all[parent]) all[parent] = list();
                    all[parent] << field;
                }
            }
            while(pair in all) {
                key = next(pair);
                val = next(pair);
                details += "!local{!{symbol}.!{key}} as {list::cat(";
                details += "!symbol(!{symbol}.!{next(val)})";
                while(field in val) details += ", !symbol(!{symbol}.!{field})";
                details += ")}\n";
            }
            return details+ret;
        }
    }

    !local{@Point_ @@name_ = @@value;} as {
        @@symbol_list = list::cat(@@value);
        !local{@@name_} as {
            !include !comptime do {
                fields = !codestring(@fields)|bb.string.split(",");
                while(i in range(fields|len)) fields[i] = fields[i]|bb.string.split(" ")|bb.string.join("");
                symbol = !codestring(@@name_);
                ret = "";
                while(field in fields) {
                    if(ret|len!=0) ret += ",";
                    ret += "!symbol(!{symbol}.!{field})";
                }
                return ret;
            }
        }
        !include !comptime do {
            fields = !codestring(@fields)|bb.string.split(",");
            while(i in range(fields|len)) fields[i] = fields[i]|bb.string.split(" ")|bb.string.join("");
            symbol = !codestring(@@name_);
            symbol_list = !codestring(@@symbol_list);
            ret = "";
            while(field in fields) ret += "!symbol(!{symbol}.!{field}) = next(!{symbol_list});\n";
            while(field in fields) ret += "!local{!{symbol}.!{field}} as {!symbol(!{symbol}.!{field})}\n";
            details = "";
            all = map();
            while(field in fields) {
                split = field|bb.string.split(".");
                if(split|len>1) {
                    parent = split[range(0,split|len-1)]|bb.string.join(".");
                    fields << parent;
                    catch(all[parent]) all[parent] = list();
                    all[parent] << field;
                }
            }
            while(pair in all) {
                key = next(pair);
                val = next(pair);
                details += "!local{!{symbol}.!{key}} as {list::cat(";
                details += "!symbol(!{symbol}.!{next(val)})";
                while(field in val) details += ", !symbol(!{symbol}.!{field})";
                details += ")}\n";
            }
            return details+ret;
        }
    }
}
