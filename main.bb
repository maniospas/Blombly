value = 1;
a = new {
    test = {this..value = 2; return this..value}
}

print(value);
print(a.test());