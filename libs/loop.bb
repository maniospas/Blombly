#macro {loop::step(@name)} as {next(#of iter(@name))}
#macro {loop::noprefix;} as {#macro(loop::step) = {step}}
