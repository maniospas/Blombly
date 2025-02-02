A = new{
    x = 1;
    y = 2;
}

clear(A);
err = do print(A.x);
catch(err) print("caught");

value = do catch(err) return true else return false;
print(value);