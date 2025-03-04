!macro {in @expr} as {as next(!anon iter(@expr))}
!macro {assert @expr;} as {if((@expr)==false) fail("Assertion failed");}
!macro {=>@expr;} as {={return @expr;}}
!macro {=>{@expr}} as {!fail "`=>{` not allowed just before a code block.\nThis is a common code editing error when enriching\nshorter implementations and thus not allowed."}
!macro {default @var=} as {catch(@var) @var=}

// define namespaces
!macro {!namespace @name {@generator}} as {
    !macro {!with @name:} as {
        //!local {!var @@@symbol(@@@arguments)=@@@expression;} as {!local{@@@symbol} as {!symbol(@name :: @@@symbol)} @@@symbol(@@@arguments)=@@@expression;}
        //!local {!var @@@symbol=@@@expression;} as {!local{@@@symbol} as {!symbol(@name :: @@@symbol)} @@@symbol=@@@expression;}
        !local {!var @@@symbol;} as {!local{@@@symbol} as {!symbol(@name :: @@@symbol)}}
        @generator
    }
}

!macro {!nameless {@generator}} as {
    {
        !include {
            !local {!var @@symbol;} as {!local{@@symbol} as {!symbol(@name :: @@symbol)}}
            @generator
        }
    }
}

!macro {yield} as {!fail "The yield command is only available within !gather brackets, and only in the form of yield @symbol;."}
!macro {!gather(@value,@operation){@block}} as {
    new{
        !include {
            !local{yield @@symbol;} as {@var @operation @@symbol;}
            @var=@value; 
            @block 
        }
        return @var;
    }
}

// include after core language definitions
final bb = new {
    !include "bb://libs/ansi"
    !include "bb://libs/monad"
    !include "bb://libs/tests"
    !include "bb://libs/collections"
    !include "bb://libs/plot"
}