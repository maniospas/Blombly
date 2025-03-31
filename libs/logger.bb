!with colors:
logger = new {
    ok(str text) = {print("[  !{bb.ansi.lightgreen}ok!{bb.ansi.reset}  ] !{text}")}
    fail(str text) = {print("[ !{bb.ansi.lightred}fail!{bb.ansi.reset} ] !{text}")}
    warn(str text) = {print("[ !{bb.ansi.yellow}warn!{bb.ansi.reset} ] !{text}")}
    info(str text) = {print("[ !{bb.ansi.lightcyan}info!{bb.ansi.reset} ] !{text}")}
}
