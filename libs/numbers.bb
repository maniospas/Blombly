#include "libs/oop"
enable oop;


abstract Version {
    final version = "Number library version: v1.0";
}

class Real(value) {
    Version:
    uses Complex;
    uses value;
    
    fn real() {return Real(value);}
    fn complex() {return Complex(value, 0);}
    fn \add(other) {return Real(value+other.value);}
    fn \sub(other) {return Real(value-other.value);}
    fn \mul(other) {return Real(value*other.value);}
    fn \div(other) {return Real(value/other.value);}
    fn \str() {return "Real("+str(value)+")";}
}

class Complex(re, im) {
    Version:
    uses Real;
    uses re;
    uses im;
    
    fn real() {
        if(im!=0)
            fail("The imaginary part is not zero.");
        return Real(re);
    }
    fn norm() {return Real(re^2+im^2);}
    fn complex() {return this;}
    fn \add(other) {other = other.complex();return Complex(re+other.re, im+other.im);}
    fn \sub(other) {other = other.complex();return Complex(re-other.re, im-other.im);}
    fn \mul(other) {other = other.complex();return Complex(re*other.re-im*other.im, re*other.im+im*other.re);}
    fn \div(other.complex()) {
        other = other.complex();
        norm = other.re^2+other.im^2;
        re = re/norm;
        im = im/norm;
        return Complex(re*other.re+im*other.im, im*other.re-re*other.im);
    }
    fn \str() {return "Complex("+str(re)+","+str(im)+")";}
}
