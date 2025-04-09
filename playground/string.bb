final String = {
    final str() => this.value;
    final len() => this.length;
    final starts(str other) => bb.string.starts(other);
    final ends(str other) => bb.string.ends(other);
    final split(str c) => this.value|bb.string.split(c);
    final contains(str other) => this.value|bb.string.index(other)!=len(this.value);
    final add(other) => new{
        String: 
        value = this..value+other.value;
        length = this..length+other.length;
    }
    defer {
        default length = len(value);
        final value=value;
        final length=length;
    }
}
final utf8(str value) => new{String:vale=value}

x = utf8("hi!", 2)+utf8("😀"); // CREATES AN ERROR
print(x|len);
print(x.contains("😀"));