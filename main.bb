back(element) = {return new {
    final element = element;
    \call(A) = {
        push(A, element);
        return A;
    }
}}

A = list();
A = A|back(1);
print(A);


buff = "";
tic = bbvm::time();
while(i in range(100000)) 
    buff = buff+" "+str(i);

toc = bbvm::time();
print(len(buff));
print((toc-tic), "sec");
print(buff[range(20)]);
