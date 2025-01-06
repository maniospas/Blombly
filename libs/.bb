!macro {in @expr} as {as next(!anon iter(@expr))}
!macro {assert @expr;} as {if((@expr)==false) fail(!stringify("Assertion failed: " @expr));}
!macro {=>@expr;} as {={return @expr;}}
!macro {=>{@expr}} as {!fail "`=>\{` not allowed just bfore a code block.\nThis is a common code editing error when enriching\nshorter implementations and thus not allowed."}
!macro {->} as {return}

// include after core language definitions
final bb = new {
    !include "libs/ansi"
    !include "libs/monad"
    !include "libs/tests"
    !include "libs/collections"
}
