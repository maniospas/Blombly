// function with only positional arguments (overriden by subsequent versions)
#macro {final::def @name(@args){@code}} as {
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
}
// function with no arguments
#macro {final::def @name(){@code}} as {
    final @name = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        @code
    }
}
// function with defaults 
#macro {final::def @name(@args | @defaults){@code}} as {
    final @name(@args) = {
        //#spec type="fn";
        //#spec name=#stringify(@name); 
        default {@defaults}
        @code
    }
}
// function with only defaults (identified because it has an assignment)
// #macro (final::def @name(@arg = @defaults){@code}) = {
//    final @name = {
//        #spec type="fn";
//        #spec name=#stringify(@name); 
//       default {@arg = @defaults}
//        @code
//    }
//}