!modify "vfs://"

f1 = "vfs://test.txt"|file;
push(f1, "Virtual data.");

f2 = "vfs://test.txt"|file;
assert bb.os.read(f2) == "Virtual data.";