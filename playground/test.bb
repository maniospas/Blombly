add(x, y) = {
    default xscale = 1;
    default yscale = 1;
    return x*xscale + y*yscale;
}
print(add(1,2 :: xscale=2;yscale=2)); // execute all code after :: and 
final xscale = 10; // all running functions in the scope also see finals
print(add(1,2));