#include "std/oop"

class Dict {
    final keys = map();
    final values = map();

    fn !put(this, k, v) {
        keys[k] = k;
        values[k] = v;
        return this;
    }
    fn !at(this, k) {
        return values[k];
    }
    fn !len(this) {
        return len(this.keys);
    }
}

fn dict() {
    return new{Dict:}
}