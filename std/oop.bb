#macro (class @name {@code}) = (final @name = {@code});
#macro (fn @name(@args){@code}) = (final @name(@args) = {@code});
#macro (module @name {@code}) = (final @name = new {@code});
#macro (overload @name(@args) {@code}) = (final ! @name(this, @args) {@code});