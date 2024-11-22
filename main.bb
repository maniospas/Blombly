myformat(x) = {return x[".3f"];}

while(not x as "Give a number:"|read|float) 
    print("Failed to convert to a float");

print("Your number is {x|myformat}");