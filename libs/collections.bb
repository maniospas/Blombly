final collection = new {
    final push_back(element) => new {
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

final string = new {
    call(value) => bbvm::str(value);
    
    final startswith(str query) => new {
        call(str search) = {
            query = this..query;
            nsearch = search|len;
            nquery = query|len;
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[i]) return false;
            return true;
        }
    }
    final endswith(str query) => new {
        call(str search) = {
            query = this..query;
            nsearch = search|len;
            nquery = query|len;
            if(nsearch<nquery) return false;
            while(i in range(nquery)) if(query[i]!=search[nsearch-nquery+i]) return false;
            return true;
        }
    }
    final index(str query) => new {
        catch(try {pos=pos; return}) pos = 0;
        call(str search) = {
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
                pos = search|string.index(query :: pos=pos);
                s = search[range(prev_pos, pos)];
                push(ret, s);
                pos += nquery;
            }
            return ret;
        }
    }
}