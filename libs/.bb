!macro {in @expr} as {as bbvm::next(!anon bbvm::iter(@expr))}
!macro {assert @expr;} as {if((@expr)==false) fail(!stringify("Assertion failed: " @expr));}
!macro {=>@expr;} as {={return @expr;}}
!macro {->} as {return}

// include after core language definitions
!include "libs/ansi"
!include "libs/monad"
!include "libs/tests"
!include "libs/collections"