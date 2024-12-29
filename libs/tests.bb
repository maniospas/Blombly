!include "libs/ansi"


!macro {test.run(@name){@code}} as {
    @result = try {@code} 
    catch(@result)
        print("[ {ansi.red}fail{ansi.reset} ] "+!stringify(@name)+"\n"+str(@result)) 
    else 
        print("[  {ansi.green}ok{ansi.reset}  ] "+!stringify(@name));
}