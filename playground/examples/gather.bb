B = !gather(list(),<<) {while(x in range(20)) {while(y in range(2,4)) if(x%y==0) yield x,y;}}
print(B);

// TODO: for parser auto add brackets for everything at the beginning