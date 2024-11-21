// `in` for loops
!macro {in @expr} as {as bbvm::next(!of bbvm::iter(@expr))}

// semitype declarations
!macro {semitype @name;} as {@name(value) = {return value. !symbol(@name)()}}