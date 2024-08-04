#include "oop"

class Finder { 
    fn contains(number) {
        i = 2;
        i_max = int(number^0.5);
        while (i <= i_max) {
            if (number%i==0) 
                return false;
            i = i + 1;
        }
        return true;
    }

    fn next() {
        while(true) {
            this.number = this.number + 1;
            if (this.contains(number)) 
                return this.number;
        }
    }
}

finder = new {Finder:number = 10;}

print(next(finder));
print(next(finder));
