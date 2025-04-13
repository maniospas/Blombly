// look at the .bbvm file to see compile-time list optimization
a = 1,2,3;
a << 6;
b = 4,5;
c = a+b;
print(c|pop);
print(c|next);
print(c);