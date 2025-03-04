A = 1,2,3,4,5;
value = !gather(0, +=) {while(i in A) if(i%2==0) yield i^2;}
print("Sum of squares !{value}");