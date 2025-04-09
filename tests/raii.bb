final Araii = new{message="Hello world!"} // make final to ensure that this is the thing cleared
defer clear(Araii); // there will be no leaks
Braii = new{Araii=Araii}
Araii.Braii = Braii;
assert Araii.message=="Hello world!"; // the actual assert will be at the end of tests


memory = bb.memory.raii();
defer clear(memory);
final Craii = memory<<new{message="Hello world!"}
Braii = new{Craii=Craii}
Craii.Braii = Braii;
assert Craii.message=="Hello world!"; // the actual assert will be at the end of tests
