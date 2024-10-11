final thread::INFO as {
    name    = "thread";
    author  = "Emmanouil Krasanakis";
    license = "Apache 2.0";
    version = "1.0";
    release = 0;
    year    = 2024;
    doc     = "
    \n Introduces the `thread::sync` macro that allows
    \n forceful synchronization of method calls. This
    \n library that may not be supported in the future.
    ";
    experimental = true;
}

#macro {thread::sync @code;} as {@ret as @code;}
