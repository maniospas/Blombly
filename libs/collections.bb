!macro {.io::username} as {["username"]}
!macro {.io::password} as {["password"]}
!macro {.io::timeout} as {["timeout"]}

collection = new {
    toback(element) => new {
        call(A) = {
            A << this..element;
            return A;
        }
    }
    transform(func) => new {
        call(A) = {
            ret = list();
            while(x in A) ret << this..func(x);
            return ret;
        }
    }
}

db(str path) => new {
    connector = sqlite(path);
    table(str table_name, str signature) = {
        this.connector << "CREATE TABLE IF NOT EXISTS !{table_name} (!{signature});";
        return new {
            insert(entry) = {
                keys = list();
                values = list();
                while(pair in entry) {
                    keys << pair|next|str;
                    values << pair|next|str;
                }
                join = bb.string.join(",");
                this...connector << "INSERT INTO !{this..table_name} (!{keys|join}) VALUES (!{values|join})";
            }
            select(str where) = {
                if(where=="*") return this...connector << "SELECT * FROM !{this..table_name}";
                return this...connector << "SELECT * FROM !{this..table_name} WHERE !{where};";
            }
        }
    }
    transaction() = {
        this.connector << "BEGIN TRANSACTION;";
        return new {
            // this is how to end the transaction
            connector = this..connector;
            call() => this...commit();
        }
    }
    commit() => this.connector << "COMMIT;";
    run(query) => this.connector << query|str;
}

memory = new {
    raii() => new {
        entries = list();
        push(obj) = {
            this.entries << obj;
            return obj;
        }
        clear() = {while(obj as this.entries|next) clear(obj)}
    }
}

os = new {
    call(str file path) => path;
    isfile(str path) => bool(file(path)/".")==false;
    read(str file path) = {
        ret = "";
        while(line in path) {
            if(ret|len|bool) ret += "\n";
            ret += line;
        }
        return ret;
    }
    transfer() = {
        src = file(from|str);
        dst = file(to|str);
        default checksum = "";
        checksum |= str;
        if(checksum|len|bool) {
            dsttext = do return dst|bb.os.read;
            catch(dsttext) dsttext = "";
            if((checksum=="*" and dsttext|len|bool) or checksum==dsttext["md5"]) return;
        }
        text = src|bb.os.read;
        target = text["md5"];
        bb.logger.info("!{target} downloaded from: !{from}");
        if(checksum|len|bool) if(checksum!="*" and checksum!=target) fail("The retreived checksum did not match the desired one: !{src|str}");
        dst << text;
        return;
    }
}

string = new {
    // can use bb.string instead of str for typecasting
    call(str value) => value;

    // hash functions exposed from openssl
    md5(arg) => arg["md5"];
    sha1(arg) => arg["sha1"];
    sha224(arg) => arg["sha224"];
    sha256(arg) => arg["sha256"];
    sha384(arg) => arg["sha384"];
    sha512(arg) => arg["sha512"];
    sha3_224(arg) => arg["sha3_224"];
    sha3_256(arg) => arg["sha3_256"];
    sha3_384(arg) => arg["sha3_384"];
    sha3_512(arg) => arg["sha3_512"];
    blake2b(arg) => arg["blake2b"];
    blake2s(arg) => arg["blake2s"];
    ripemd160(arg) => arg["ripemd160"];
    whirlpool(arg) => arg["whirlpool"];
    sm3(arg) => arg["sm3"];

    join(str delimiter) => new {
        call(strlist) = {
            ret = "";
            while(entry in strlist) {
                if(ret|len!=0) ret += this..delimiter;
                ret += entry;
            }
            return ret;
        }
    }

    // common string manipulation methods
    starts(str query) => new {
        call(str search) = {
            assert args|len == 0;
            query = this..query;
            nsearch = len(search);
            nquery = len(query);
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[i]) return false;
            return true;
        }
    }
    ends(str query) => new {
        call(str search) = {
            assert args|len == 0;
            query = this..query;
            nsearch = len(search);
            nquery = len(query);
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[nsearch-nquery+i]) return false;
            return true;
        }
    }
    index(str query) => new {
        default pos = 0 else pos = int(pos);
        call(str search) = {
            assert len(args) == 0;
            query = this..query;
            pos = this.pos;
            nsearch = len(search);
            nquery = len(query);
            while(i in range(pos, nsearch-nquery+1)) {
                different = do while(j in range(nquery)) if(query[j]!=search[i+j]) return true;
                catch(different) return i;
            }
            return nsearch;//fail("Index not found");
        }
    }
    split(str query) => new {
        assert len(args) == 0;
        default maxsplits = 0;
        call(str search) = {
            query = this..query;
            nsearch = len(search);
            nquery = len(query);
            if(nquery==0) fail("Cannot split on a zero-length string");
            ret = list();
            pos = 0;
            while(pos<nsearch) {
                prev_pos = pos;
                pos = search|bb.string.index(query :: pos=pos);
                s = search[range(prev_pos, pos)];
                ret << s;
                pos += nquery;
            }
            return ret;
        }
    }
}
