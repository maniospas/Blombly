A = 1,2,3,4;
bbvm::assert A[1] == 2;
bbvm::assert A|next == 1;
bbvm::assert A|next == 2;
bbvm::assert A|pop == 4;
bbvm::assert A|pop == 3;
bbvm::assert A|len == 0;
push(A, 5);
bbvm::assert A|pop == 5;
