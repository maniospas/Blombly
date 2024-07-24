least = 4;
i = 0;
result = try while (i<=100) {
    i = i + 3;
    if (i>=least) 
        return i;
}
print("Finished searching.");
catch (result) 
    fail("Found nothing: "+str(result));
print("The least multiple of 3 in range ["+str(least)+", 100] is: "+str(result));