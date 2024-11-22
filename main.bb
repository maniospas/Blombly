myformat(x) = {return x[".3f"];}

// `as` is the same as `=` but returns whether the assignment
// was succesful instead of creating an exception on failure
while(not x as "Give a number:"|read|float) {}

print("Your number is {x|myformat}");