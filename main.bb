final bias = 1;
final sum = {print("exec");return(add(x,bias));}
final sum2 = {print("exec2");return(add(x1,y));}
args = {x=1;y=2;}
final x1 = sum(args);
x2 = sum2(args);
x3 = sum(args);
x4 = sum2(args);
print(x1);
print(x2);
print(x3);
print(x4);