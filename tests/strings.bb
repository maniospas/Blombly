a = "0";
b = "1";

assert a <= b;
assert a < b;
assert a == a;
assert a != b;
assert b > a;
assert b >= a;

hi = "this is a nice day 😀";
assert hi|bb.string.base64 == "dGhpcyBpcyBhIG5pY2UgZGF5IPCfmIA="; // from https://base64.guru/converter
assert hi|bb.string.base64|bb.string.debase64 == hi;
assert len(hi|bb.string.deutf8) > len(hi);
assert hi|bb.string.deutf8|bb.string.utf8 == hi;
assert hi|bb.string.deutf8|bb.string.deescape|len > hi|bb.string.deutf8|len;
assert hi|bb.string.deutf8|bb.string.deescape|bb.string.escape|bb.string.utf8 == hi;
