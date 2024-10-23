
// class definition
#macro {def::class @name(@args) {@code}} as {
    def::fn @name(@args) {
        #spec type="class";
        #spec name=#stringify(@name);
        return new {
            final type = @name;
            def::uses @name;
            @code
        }
    }
}

// uses definitition
#macro {def::uses @name;} as {
    final @name = @name;
}

// abstract definition
#macro {def::abstract @name {@code}} as {
    final @name = {
        #spec type="abstract";
        #spec name=#stringify(@name); 
        @code
    }
}

// module definition
#macro {def::module @name {@code}} as {
    final @name = new {
        final type="module";
        final name=#stringify(@name); 
        @code
    }
}

