memory = bb.memory.raii();
defer clear(memory);

A = memory+new{}
B = memory+new{A=A}
A.B = B;
