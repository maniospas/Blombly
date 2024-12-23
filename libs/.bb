!macro {in @expr} as {as bbvm::next(!of bbvm::iter(@expr))}
!macro {semitype @name;} as {@name(value) = {return value. !symbol(@name)()}}
!macro {:=} as {= try}
!macro {uses @name;} as {final @name = @name;}
!macro {assert @expr;} as {if((@expr)==false) fail(!stringify("Assertion failed: " @expr));}

final back(element) = {return new {
    final element = element;
    \call(A) = {
        bbvm::push(A, element);
        return A;
    }
}}
