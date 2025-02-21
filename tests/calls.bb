foo(x,y) = {
    default bias = 0;
    return x+y+bias;
}

assert foo(1,2 :: bias=3) == 6;

failed_on_extra_args = do catch(foo(1,2,3)) return true else return false;
assert failed_on_extra_args;