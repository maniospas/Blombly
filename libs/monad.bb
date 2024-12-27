!macro {semitype @name;} as {@name(value) = {return value. !symbol(@name)()}}

!macro {monad(@symbol)} as {new {final @symbol=@symbol}}
!macro {monad(@symbol);} as {new {final @symbol=@symbol}}
!macro {monad(@symbol).} as {new {final @symbol=@symbol}.}
