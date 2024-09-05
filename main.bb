#include "libs/std"
enable std;

fn Complex(re, im) {
    return new{
        uses Complex;
        uses re;
        uses im;
        fn \add(other) {
            return Complex(re+other.re, im+other.im);
        }
        fn \sub(other) {
            return Complex(re-other.re, im-other.im);
        }
        fn \mul(other) {
            return Complex(re*other.re-im*other.im, re*other.im+im*other.re);
        }
        fn \str() {
            return "("+str(re)+","+str(im)+")";
        }
    }
}

x = Complex(1, 1);
y = Complex(2, 1);

z = x*y;
print(z);