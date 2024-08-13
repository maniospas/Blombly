#macro (class @name {@code}) = (
    final @name = {
        //#spec type="class";
        //#spec name=#stringify(@name); 
        @code
    }
);
#macro (fn @name(@args){@code}) = (
    final @name(@args) = {
        #spec type="fn";
        #spec name=#stringify(@name); 
        @code
    }
);
#macro (module @name {@code}) = (
    final @name = new {
        final type="module";
        final name=#stringify(@name); 
        @code
    }
);