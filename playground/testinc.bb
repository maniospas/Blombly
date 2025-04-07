// To see this in action first compile the library per: 
// ./blombly playground/testinclude.bb --library--norun
!include "playground/testinclude.bbvm"

adder = create_adder(inc=2);
print(adder(1,2));
