!modify "vfs://"

A = "vfs://test.txt"|file;
push(A, "Virtual data.");

B = "vfs://test.txt"|file;
print(bb.os.read(B));