!macro {in @expr} as {as bbvm::next(!anon bbvm::iter(@expr))}
!macro {assert @expr;} as {if((@expr)==false) fail(!stringify("Assertion failed: " @expr));}
!macro {=>@expr;} as {={return @expr;}}
!macro {=>{@expr}} as {!fail "`=>` followed by a code block is likely a code editing error and not allowed."}
!macro {->} as {return}

// include after core language definitions
!include "libs/ansi"
!include "libs/monad"
!include "libs/tests"
!include "libs/collections"