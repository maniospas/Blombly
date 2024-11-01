#macro {= err::invalid(@message)} as {as try {std::fail(@message)}}
#macro {as err::invalid} as {#fail "Use `var=err::invalid(@message)` instead of `var as err::invalid(@message)`"}
#macro {return err::invalid(@message);} as {@tmp as try {std::fail(@message)} return @tmp;}
