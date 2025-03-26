final errorObj = new {}
add(x,y) => x+y;
sideeffect() = {
    errorObj.x = 1+"foo";
    return 1;
}
overwrite_nonerror(x,y) = {
    x = x+y;
    return 1;
}
skipping_errors_is_sideeffect() = {
    sideeffect();
    return 1;
}

err = add(1, "foo");
err_sideeffect = sideeffect();
err_overwrite_nonerror = overwrite_nonerror(1, "foo");
err_skipped = skipping_errors_is_sideeffect();
assert do catch(err) return true else return false;
assert do catch(err_sideeffect) return true else return false;
assert do catch(err_overwrite_nonerror) return false else return true;
assert do catch(err_skipped) return true else return false;
