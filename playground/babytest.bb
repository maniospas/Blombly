A = new{
    x = 1;
    y = 2;
}

clear(A);
err = try print(A.x);
catch(err) {print("caught")}

value = try catch(err) return true else return false;
print(value);