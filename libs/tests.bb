!include "libs/ansi"


!macro {bbvm::test(@name){@code}} as {
    @result = try {@code return} 
    catch(@result) print("[ {ansi.red}fail{ansi.reset} ] "+!stringify(@name)+"\n"+str(@result)) 
    else print("[  {ansi.green}ok{ansi.reset}  ] "+!stringify(@name));
}
