#include "std/oop"

class Dict {
    final keys = list();
    final values = map();
    final test = "hello";

    fn \put(k, v) {
        push(keys, k);
        values[k] = v;
        return this;
    }
    fn \at(k) {
        return values[k];
    }
    fn \len() {
        return len(keys);
    }

    fn \iter() {
        return iter(keys);
    }
}

fn dict() {
    return new{Dict:}
}