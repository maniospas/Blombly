#macro {type::cast @name;} as {@name(value) = {return value. #symbol(@name)()}}

#macro {= type::err(@message)} as {as try {bbv::fail(@message)}}
#macro {as type::invalid} as {#fail "Use `var=type::invalid(@message)` instead of `var as type::invalid(@message)`"}
#macro {return err::invalid(@message);} as {@tmp as try {bbv::fail(@message)} return @tmp;}
