!macro {in @expr} as {as next(!anon iter(@expr))}
!macro {assert @expr;} as {if((@expr)==false) fail("Assertion failed");}
!macro {=>@expr;} as {={return @expr;}}
!macro {=>{@expr}} as {!fail "`=>{` not allowed just bfore a code block.\nThis is a common code editing error when enriching\nshorter implementations and thus not allowed."}
!macro {default @var=} as {catch(@var) @var=}

// include after core language definitions
final bb = new {
    !include "bb://libs/ansi"
    !include "bb://libs/monad"
    !include "bb://libs/tests"
    !include "bb://libs/collections"
    !include "bb://libs/plot"
}
