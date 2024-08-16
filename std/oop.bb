// abstract definition
#macro (std::abstract @name {@code}) = (
    final @name = {
        //#spec type="abstract";
        //#spec name=#stringify(@name); 
        @code
    }
);
// function with only positional arguments (overriden by subsequent versions)
#macro (std::fn @name(@args){@code}) = (
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
);
// function with defaults 
#macro (std::fn @name(@args | @defaults){@code}) = (
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@defaults}
        @code
    }
);
// function with only defaults (identified because it has an assignment)
#macro (std::fn @name(@arg = @defaults){@code}) = (
    final @name = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@arg = @defaults}
        @code
    }
);
// module definition
#macro (std::module @name {@code}) = (
    final @name = new {
        //final type="module";
        //final name=#stringify(@name); 
        @code
    }
);