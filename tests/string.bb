A = "I like banana!";
assert A|bb.string.starts("I like");
assert A|bb.string.ends("banana!");
assert A|bb.string.split(" ")|len == 3;
assert A|bb.string.index("like") == 2;
assert A|bb.string.index("an" :: pos=9) == 10;
