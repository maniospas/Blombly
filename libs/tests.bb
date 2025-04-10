!macro {test(@name){@code}} as {
    @tic = time();
    @result = do {@code return true} 
    @toc = time();
    catch(@result) {
        bb.logger.fail(!stringify(@name) :: eta=@toc-@tic);
        fail(@result);
    }
    else bb.logger.ok(!stringify(@name) :: eta=@toc-@tic);
}

