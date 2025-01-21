A = "I like banana!";
assert A|bb.string.starts("I like");
assert A|bb.string.ends("banana!");
assert A|bb.string.ends("w")==false;
assert A|bb.string.index("like") == 2;
assert A|bb.string.index("an" :: pos=9) == 10;
assert A|bb.string.split(" ")|len == 3;