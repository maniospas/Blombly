# Assembly

This is the `blombly` assembly language, in which the main language is compiled.
Compiled programs in this language can run in the namesake virtual machine. The
`#` symbol is used to indicate its usage as an unused value reserved for a uniform
representation.

| Command           | Description |
|-------------------|-------------|
| BUILTIN res value | Sets to variable `res` the given value of a built-in type. The latter can either be a string in quotations (e.g., "Hello world!"), or an integer prefixed with I (e.g., I3), or a float prefixed with F (e.g., F0.5) |
| return # var      | Returns the value of the variable `var` from a block of code. |
| inline res var    | Runs the block of code referenced by the variable `var` inline and stores any returned value in `res`. |
| CALL res ctx var  | Runs the block of code `var` with keyword argument context determined by `ctx` (this can be `#` for a run without arguments). The returned value is stored in `res`. This call can be parallelized, in which case `res` can obtain a reference to a running thread, which will be joined whenever the value is needed in subsequent computations. |
| BEGIN res         | Declares the beginning of a code block, whose reference is stored in the variable `res`. |
| END               | Declares the end of a code block (begin-end declarations can be nested). |
| FINAL res         | Declares that the current value of the variable `res` is final and can never change in the future. Only final variables are visible to called blocks. |
| BEGINFINAL res    | A shorthand for BEGIN res that immediately sets it to final, to not need FINAL res after its END. This command is only useful to simplify parsing. |