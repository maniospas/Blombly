!macro {in @expr} as {as bbvm::next(!of bbvm::iter(@expr))}
!macro {semitype @name;} as {@name(value) = {return value. !symbol(@name)()}}
!macro {:=} as {= try return}
!macro {uses @name;} as {final @name = @name;}