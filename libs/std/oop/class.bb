
// class definition
#macro (std::class @name(@args) {@code}) = {
    final @name(@args) = {
        #spec type="class";
        #spec name=#stringify(@name);
        return new {
            final type = @name;
            std::uses @name;
            @code
        }
    }
}

// uses definitition
#macro (std::uses @name;) = {
    final @name = @name;
}

// abstract definition
#macro (std::abstract @name {@code}) = {
    final @name = {
        #spec type="abstract";
        #spec name=#stringify(@name); 
        @code
    }
}

// module definition
#macro (std::module @name {@code}) = {
    final @name = new {
        final type="module";
        final name=#stringify(@name); 
        @code
    }
}

