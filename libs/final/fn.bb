// function with only positional arguments (overriden by subsequent versions)
#macro {final::fn @name(@args){@code}} as {
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
}
// function with no arguments
#macro {final::fn @name(){@code}} as {
    final @name = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
}
// function with defaults 
#macro {final::fn @name(@args | @defaults){@code}} as {
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@defaults}
        @code
    }
}
// function with only defaults (identified because it has an assignment)
// #macro (final::fn @name(@arg = @defaults){@code}) = {
//    final @name = {
//        #spec type="fn";
//        #spec name=#stringify(@name); 
//       default {@arg = @defaults}
//        @code
//    }
//}