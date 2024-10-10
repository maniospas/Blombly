final docs::INFO as {
    name    = "docs";
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
    \n Then import both docs and library and call one 
    \n of the following methods:
    \n
    \n - docs::help(library::INFO); 
    \n   Prints the details of the library.
    ";
} // ---------------------------------------------------

final docs::str(info) = {
    final hbar = "-------------------------------------------------------------";
    info:
    info = hbar;
    info = info + "\n{name} {version}.{release}";
    info = info + "\nCopyright (C) {year}, {author}";
    info = info + "\n{license} license";
    info = info + "\n" + doc;
    info = info + "\n" + hbar;
    return info;
}

#macro {docs::include(@lib);} as {
    #include #stringify(libs/ @lib)
    @info = new{#symbol(@lib ::INFO):}
}

#macro {docs::help(@lib);} as {
    @info = #symbol(@lib ::INFO);
    std::print(docs::str(@info));
}