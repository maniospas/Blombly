
// class definition
#macro {final::class @name {@code}} as {
    final @name = {
        //#spec type="class";
        //#spec name=#stringify(@name);
        return new {
            //final type = @name;
            //final::use @name;
            @code
        }
    }
}

// class definition
#macro {final::class @name {@code}} as {
    final @name = {
        //#spec type="class";
        //#spec name=#stringify(@name);
        return new {
            //final type = @name;
            //final::use @name;
            @code
        }
    }
}

// uses definitition
#macro {final::use @name;} as {
    final @name = @name
;
}

// uses with semitypes
#macro {final::use @name|@semitype;} as {
    final @name = @name|@semitype;
}


// abstract definition
#macro {final::abstract @name {@code}} as {
    final @name = {
        //#spec type="abstract";
        //#spec name=#stringify(@name); 
        @code
    }
}

// module definition
#macro {final::struct @name {@code}} as {
    final @name = new {
        //final type="struct";
        //final name=#stringify(@name); 
        @code
    }
}

