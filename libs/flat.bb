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
            symbol = !codestring(@@name_);
            ret = "";
            while(field in fields) {
                if(ret|len!=0) ret += ",";
                ret += "!symbol(!{symbol}.!{field})";
            }
            while(field in fields) ret += "!local{!{symbol}.!{field}} as {!symbol(!{symbol}.!{field})}\n";
            return ret;
        }
    }
    !local{@Point_ @@name_ = @@value;} as {
        @@symbol_list = list::cat(@@value);
        !local{@@name_} as {
            !include !comptime do {
                fields = !codestring(@fields)|bb.string.split(",");
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
            symbol = !codestring(@@name_);
            symbol_list = !codestring(@@symbol_list);
            ret = "";
            while(field in fields) ret += "!symbol(!{symbol}.!{field}) = next(!{symbol_list});\n";
            while(field in fields) ret += "!local{!{symbol}.!{field}} as {!symbol(!{symbol}.!{field})}\n";
            return ret;
        }
    }
}
