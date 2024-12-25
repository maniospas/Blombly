!macro {in @expr} as {as bbvm::next(!anon bbvm::iter(@expr))}
!macro {semitype @name;} as {@name(value) = {return value. !symbol(@name)()}}
!macro {assert @expr;} as {if((@expr)==false) fail(!stringify("Assertion failed: " @expr));}
//!macro {:=} as {= try}

final back(element) = {return new {
    final element = element;
    call(A) = {
        bbvm::push(A, element);
        return A;
    }
}}

final apply(func) = {return new {
    final func = func;
    call(A) = {
        ret = list();
        while(x in A)
            push(ret, func(x));
        return ret;
    }
}}

!macro {=>@expr;} as {={return @expr;}}
!macro {->} as {return}
!macro {monad(@symbol)} as {new {final @symbol=@symbol}}
!macro {monad(@symbol);} as {new {final @symbol=@symbol}}
!macro {monad(@symbol).} as {new {final @symbol=@symbol}.}