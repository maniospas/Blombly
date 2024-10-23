final symb::INFO as {
    name    = "symb";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces a mapping between various symbols and
    \n coding notation. This may be used by various other
    \n packages
    \n
    \n  Symbol    Replaced by
    \n --------  --------------
    \n  λ         symb::lambda    
    \n  π         symb::pi    
    ";
}

#macro {λ} as {symb::lambda}
#macro {π} as {symb::pi}
