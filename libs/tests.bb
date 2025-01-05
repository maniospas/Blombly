!macro {test(@name){@code}} as {
    @result = try {@code return} 
    catch(@result) print("[ {bb.ansi.red}fail{bb.ansi.reset} ] "+!stringify(@name)+"\n"+str(@result)) 
    else print("[  {bb.ansi.green}ok{bb.ansi.reset}  ] "+!stringify(@name));
}
