// build automation - enable some access and modification rights needed to grab dependencies from the web
!access "https://raw.githubusercontent.com/"
!modify "libs/download/"

// let comptime prepare everything - inclu
!comptime(bb.os.transfer("libs/download/html.bb", "https://raw.githubusercontent.com/maniospas/Blombly/refs/heads/main/libs/html.bb"));

// do some stuff with your library - it may have comptime internally too, but !access and !modify are not allowed there (yours are fixed)
!include "libs/download/html"
