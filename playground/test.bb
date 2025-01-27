final A = new{}

add(float x, float y) = { // converts arguments to floats
    print("Trying to add: !{x} and !{y}");
    return 0;
}
x = "First number:"|read;
y = "Second number:"|read;
z = add(x, y);
catch(z) print("Operation failed");
print(z);