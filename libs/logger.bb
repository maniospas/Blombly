logger = new {
    \eta = {catch(eta)return ""; eta |=float; if(eta<0.001) return ""; return "["+bb.ansi.yellow + str(eta[".3f"]) + " sec" + bb.ansi.reset+"] "}
    ok(str text) = {eta = do this\eta: print("[  !{bb.ansi.lightgreen}ok!{bb.ansi.reset}  ] !{eta}!{text}")}
    fail(str text) = {eta = do this\eta: print("[ !{bb.ansi.lightred}fail!{bb.ansi.reset} ] !{eta}!{text}")}
    warn(str text) = {eta = do this\eta: print("[ !{bb.ansi.yellow}warn!{bb.ansi.reset} ] !{eta}!{text}")}
    info(str text) = {eta = do this\eta: print("[ !{bb.ansi.lightcyan}info!{bb.ansi.reset} ] !{eta}!{text}")}
}
