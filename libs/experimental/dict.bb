dict::missing as new{}

dict() = {
    return new {
        final values = map();
        final keys = list();
        
        final erase(key) = {values[key] = dict::missing;return this}
        final \iter() = {x = iter(keys);return x}
        final \at(key) = {return values[key]}
        final \put(key, value) = {
            exists = _ as (this.values)[key];
            if(not exists) 
                push(keys, key);
            values[key] = value;
            return;
        }
    }
}
