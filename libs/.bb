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


final ansi = new {
    final black = "\e[0;30m";
    final red = "\e[0;31m";
    final green = "\e[0;32m";
    final yellow = "\e[0;33m";
    final blue = "\e[0;34m";
    final purple = "\e[0;35m";
    final cyan = "\e[0;36m";
    final white = "\e[0;37m";
    final lightblack = "\e[0;90m";
    final lightred = "\e[0;91m";
    final lightgreen = "\e[0;92m";
    final lightyellow = "\e[0;93m";
    final lightblue = "\e[0;94m";
    final lightpurple = "\e[0;95m";
    final lightcyan = "\e[0;96m";
    final lightwhite = "\e[0;97m";
    final reset = "\e[0m";
}



!macro {=>@expr;} as {={return @expr;}}
!macro {->} as {return}
!macro {monad(@symbol)} as {new {final @symbol=@symbol}}
!macro {monad(@symbol);} as {new {final @symbol=@symbol}}
!macro {monad(@symbol).} as {new {final @symbol=@symbol}.}
!macro {bbvm::test(@name){@code}} as {
    @result = try {@code} 
    catch(@result)
        print("[ {ansi.red}fail{ansi.reset} ] "+!stringify(@name)+"\n"+str(@result)) 
    else 
        print("[  {ansi.green}ok{ansi.reset}  ] "+!stringify(@name));
}