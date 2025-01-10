start = "start:"|read|int;
end = "end:  "|read|int;
i = 0;
result = try while (i <= end) {
    i = i + 3;
    if (i >= start) return i;
}
print("Finished searching.");
catch (result) fail("Found nothing: {result|str}");
print("The least multiple of 3 in range [{start}, {end}] is: {result}");