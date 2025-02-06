tic = time();


starts(query) => new {
    call(search) = {
        query = this..query;
        nsearch = len(search);
        nquery = len(query);
        if(nsearch<nquery) return false;
        while(i in range(nquery)) if(query[i]!=search[i]) return false;
        return true;
    }
}

text = "I like banana";
applied = starts("I like");
while(i in range(300000)) result = text|applied;

print(result);

print(time()-tic);