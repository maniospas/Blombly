#include "std/oop"  // for the module and fn macros (more later)

module mylib {
	final name = "mylib";
	final author = "Emmanouil Krasanakis";
	final doc = "This is a simple numeric interface";
	final version = 1.0;

    fn add(x, y) {
		#spec doc = "Computes an addition in "+name;
        #spec version = version;
        return x + y;
    }
    fn abs(x) {
		#spec doc = "Computes the absolute value in "+name;
        #spec version = version;
        if(x<0)
            return 0-x;
        return x;
    }
    final tests = {
        if(add(1,2)!=3)
            fail("Invalid addition");
        if(abs(0-1)!=1)
            fail("Invalid abs");
        print("Tests successful.");
    }
}