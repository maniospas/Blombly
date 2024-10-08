
// class definition
#macro (oop::class @name(@args) {@code}) = {
    final @name(@args) = {
        #spec type="class";
        #spec name=#stringify(@name);
        return new {
            final type = @name;
            oop::uses @name;
            @code
        }
    }
}

// uses definitition
#macro (oop::uses @name;) = {
    final @name = @name;
}

// abstract definition
#macro (oop::abstract @name {@code}) = {
    final @name = {
        #spec type="abstract";
        #spec name=#stringify(@name); 
        @code
    }
}

// module definition
#macro (oop::module @name {@code}) = {
    final @name = new {
        final type="module";
        final name=#stringify(@name); 
        @code
    }
}

