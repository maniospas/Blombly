// abstract definition
#macro (abstract @name {@code}) = (
    final @name = {
        //#spec type="abstract";
        //#spec name=#stringify(@name); 
        @code
    }
);

// function with defaults 
#macro (fn @name(@args | @defaults){@code}) = (
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@defaults}
        @code
    }
);
// function with only defaults (identified because it has an assignment)
#macro (fn @name(@arg = @defaults){@code}) = (
    final @name = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@arg = @defaults}
        @code
    }
);
// function with only posiitional arguments
#macro (fn @name(@args){@code}) = (
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
);
// module definition
#macro (module @name {@code}) = (
    final @name = new {
        //final type="module";
        //final name=#stringify(@name); 
        @code
    }
);