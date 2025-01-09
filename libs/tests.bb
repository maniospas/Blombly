!macro {test(@name){@code}} as {
    @result = try {@code return} 
    catch(@result) bb.logger.fail(!stringify(@name)+"\n"+str(@result)) 
    else bb.logger.ok(!stringify(@name));
}
