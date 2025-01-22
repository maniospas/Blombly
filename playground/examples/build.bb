// build automation - enable some access and modification rights needed to grab dependencies from the web
!access "https://raw.githubusercontent.com/"
!modify "libs/download/"

// let comptime prepare everything
!comptime(bb.os.transfer(
    to="libs/download/html.bb";
    from="https://raw.githubusercontent.com/maniospas/Blombly/refs/heads/main/libs/html.bb";
    checksum="d369d06ae1e60992b12f1417e60817d5"
));

// do some stuff with your library - it may have comptime internally too, but !access and !modify are not allowed there (yours are fixed)
//!include "libs/download/html"
