final collection = new {
    final toback(element) => new {
        call(A) = {
            push(A, this..element);
            return A;
        }
    }
    final transform(func) => new {
        call(A) = {
            ret = list();
            while(x in A) push(ret, this..func(x));
            return ret;
        }
    }
}

final logger = new {
    final ok(str text) = {print("[  {bb.ansi.lightgreen}ok{bb.ansi.reset}  ] {text}")}
    final fail(str text) = {print("[ {bb.ansi.lightred}fail{bb.ansi.reset} ] {text}")}
    final warn(str text) = {print("[ {bb.ansi.yellow}warn{bb.ansi.reset} ] {text}")}
    final info(str text) = {print("[ {bb.ansi.lightcyan}info{bb.ansi.reset} ] {text}")}
}

final memory = new {
    final raii() => new {
        final entries = list();
        final add(obj) = {
            push(this.entries, obj);
            return obj;
        }
        final clear() = {
            while(obj as this.entries|next) clear(obj);
        }
    }
}

final os = new {
    final call(str file path) => path;
    final isfile(str path) => bool(file(path)/".")==false;
    final read(str file path) = {
        ret = "";
        while(line in path) {
            if(ret|len|bool) ret += "\n";
            ret += line;
        }
        return ret;
    }
    final transfer() = {
        src = from|str|file;
        dst = to|str|file;
        default cheksum = "";
        checksum |= str;
        if(checksum|len|bool) {
            dsttext = try return dst|bb.os.read;
            catch(dsttext) dsttext = "";
            if(checksum==dsttext["md5"]) return true;
        }
        text = src|bb.os.read;
        if(checksum|len|bool) if(checksum!=text["md5"]) fail("Mismatching checksum: {src|str}");
        push(dst, text);
        return true;
    }
}

final string = new {
    // can use bb.string instead of str for typecasting
    call(str value) => value;

    // hash functions exposed from openssl
    final md5(arg) => arg["md5"];
    final sha1(arg) => arg["sha1"];
    final sha224(arg) => arg["sha224"];
    final sha256(arg) => arg["sha256"];
    final sha384(arg) => arg["sha384"];
    final sha512(arg) => arg["sha512"];
    final sha3_224(arg) => arg["sha3_224"];
    final sha3_256(arg) => arg["sha3_256"];
    final sha3_384(arg) => arg["sha3_384"];
    final sha3_512(arg) => arg["sha3_512"];
    final blake2b(arg) => arg["blake2b"];
    final blake2s(arg) => arg["blake2s"];
    final ripemd160(arg) => arg["ripemd160"];
    final whirlpool(arg) => arg["whirlpool"];
    final sm3(arg) => arg["sm3"];
    
    // common string manipulation methods
    final starts(str query) => new {
        assert args|len == 0;
        call(str search) = {
            assert args|len == 0;
            query = this..query;
            nsearch = search|len;
            nquery = query|len;
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[i]) return false;
            return true;
        }
    }
    final ends(str query) => new {
        assert args|len == 0;
        call(str search) = {
            assert args|len == 0;
            query = this..query;
            nsearch = search|len;
            nquery = query|len;
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[nsearch-nquery+i]) return false;
            return true;
        }
    }
    final index(str query) => new {
        assert args|len == 0;
        catch(try {pos=pos; return}) pos = 0;
        call(str search) = {
            assert args|len == 0;
            query = this..query;
            pos = this.pos;
            nsearch = search|len;
            nquery = query|len;
            while(i in range(pos, nsearch-nquery+1)) {
                different = try while(j in range(nquery)) if(query[j]!=search[i+j]) return true;
                catch(different) return i;
            } 
            return nsearch;//fail("Index not found");
        }
    }
    final split(str query) => new {
        assert args|len == 0;
        default maxsplits = 0;
        call(str search) = {
            query = this..query;
            nsearch = search|len;
            nquery = query|len;
            if(nquery==0) fail("Cannot split on a zero-length string");
            ret = list();
            pos = 0;
            while(pos<nsearch) {
                prev_pos = pos;
                pos = search|bb.string.index(query :: pos=pos);
                s = search[range(prev_pos, pos)];
                push(ret, s);
                pos += nquery;
            }
            return ret;
        }
    }
}