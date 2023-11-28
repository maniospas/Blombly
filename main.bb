obj = new(x=new(x=1;self);self);
print(obj.x.x);
obj.x.x = 2;
print(obj.x.x);