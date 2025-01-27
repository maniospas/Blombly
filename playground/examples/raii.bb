memory = bb.memory.raii();
defer clear(memory);

A = memory<<new{message="Hello world!"}
B = memory<<new{A=A} // add at least one object in the memory to break the cyclic reference.
A.B = B;
print(A.message);