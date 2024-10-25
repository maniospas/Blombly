
// class definition
#macro {def::class @name {@code}} as {
    final @name = {
        //#spec type="class";
        //#spec name=#stringify(@name);
        return new {
            //final type = @name;
            //def::use @name;
            @code
        }
    }
}

// class definition
#macro {def::class @name {@code}} as {
    final @name = {
        //#spec type="class";
        //#spec name=#stringify(@name);
        return new {
            //final type = @name;
            //def::use @name;
            @code
        }
    }
}

// uses definitition
#macro {def::use @name;} as {
    final @name = @name
;
}

// uses with semitypes
#macro {def::use @name|@semitype;} as {
    final @name = @name|@semitype;
}


// abstract definition
#macro {def::abstract @name {@code}} as {
    final @name = {
        //#spec type="abstract";
        //#spec name=#stringify(@name); 
        @code
    }
}

// module definition
#macro {def::struct @name {@code}} as {
    final @name = new {
        //final type="struct";
        //final name=#stringify(@name); 
        @code
    }
}

