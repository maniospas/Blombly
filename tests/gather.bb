A = 1,2,3;
Asum = !gather(0,+=) {while(i in A) yield i;}
assert Asum==6;