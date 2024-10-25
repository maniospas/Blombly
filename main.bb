#include "libs/def"
def::simplify;

abstract Animal {\name = name|str;}
abstract Friend {final \name = "My little {name|str}";}
abstract Feral  {final \name = "A wild {name|str}";}

abstract Dog {
	fn bark() {
		return "{this\name} barks: woof!";
	}
}

rex = new{
	name="Rex";
	Animal:
	Friend:
	Dog:
}

bela = new{
	name="Bela";
	Feral:
	default rex: // fallback to rex for everything not defined
}

print(bela.bark());
