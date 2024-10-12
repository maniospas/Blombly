final env::INFO as {
    name    = "env";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces version control and documentation
    \n capabilities for other libraries. In particular,
    \n new libraries should contain a library::INFO
    \n code block that sets the following variables:
    \n name, author, license, version, release, year.
    \n Then import both env and library and call one 
    \n of the following methods:
    \n
    \n - env::help(library::INFO); 
    \n   Prints the details of the library.
    ";
} // ---------------------------------------------------

final env::hbar = "-------------------------------------------------------------";

final env::str(info) = {
    info:
    info = env::hbar;
    info = info + "\n{name} {version}.{release}";
    info = info + "\nCopyright (C) {year}, {author}";
    info = info + "\n{license} license";
    info = info + "\n" + doc;
    info = info + "\n" + env::hbar;
    return info;
}

final env::dependencies = list(new{env::INFO:});

// include a library
#macro {env::include(@lib);} as {
    #include #stringify(libs/ @lib)
    @info = new{#symbol(@lib ::INFO):}
    push(env::dependencies, @info);
}

// include a library given some required features
#macro {env::include(@lib|@code);} as {
    #include #stringify(libs/ @lib)
    @info = new{#symbol(@lib ::INFO):}
    push(env::dependencies, @info);
    // create a code context in which to check for versions
    new {
        @code;
        // check whether imnported library satisfies the version or release
        while(dependency as next(#of iter(env::dependencies))) 
            if(dependency.name==@info.name) { 
                if((version as version) and (dependency.version!=version))
                    fail("Incompatible versions for library {@lib}:
                          \nimported version is {dependency.version} but {version} is required.");
                if((minrelease as minrelease) and (dependency.release<minrelease))
                    fail("Incompatible versions for library {@lib} version {version}:
                          \nimported minor release is {dependency.release} but a minimum of {minrelease} is required.");
            }
    }
}

// print some help for the given library
#macro {env::help(@lib);} as {
    @info = #symbol(@lib ::INFO);
    print(env::str(@info));
}

// text justification
final env::ljust(text) = {
    text = str(text);
    default size = 20;
    text = text + " "*(size-len(text));
    return text;
}

// print the versions of all dependencies
final env::versions() = {
    desc = env::hbar + "\n";
    desc = desc + env::ljust("Library") + " Version\n";
    while(dependency as next(#of iter(env::dependencies))) 
        desc = desc + "{env::ljust(dependency.name)} {dependency.version}.{dependency.release}\n";
    desc = desc + env::hbar;
    print(desc);
}
