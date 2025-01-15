final Num = {
    int => this.value;
    lt(other) => this.value < other.value;
    str => "num !{this.value}";
}
num(x) => new {Num:value=x}

A = 5,2,3,4,1;
A = A|bb.collection.transform(num);


n = A|len;
while(i in range(n)) {
    while(j in range(i+1, n)) if(A[j]<A[i]) {
        tmp = A[i];
        A[i]=A[j];
        A[j]=tmp;
    }
}
assert A[0]|int==1;