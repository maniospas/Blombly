A = 1,2,3;
Asum = !gather(0,+=) {while(i in A) return i;}
assert Asum==6;