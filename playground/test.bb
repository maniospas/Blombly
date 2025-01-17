tic = time();
ret = 0;
while(i in range(1000000000)) ret += i;
print(time()-tic);
print(ret);

// running time with blombly v1.18.0   88 sec
// direct equivalent in Python 3.12.7  43 sec
// direct equivalent in C++            0.27 sec
// prototype of improved memory model  2.4 sec (volatile everywhere, so some optimization may be ignored)
// adding safeptrs from prototype      99 sec (10% slowdown, expecting this to be the worst case slowdown for non-improved code, 3 sec faster if no safety checks)


// * C++ and prototype optimized with -O3, blombly is always optimized with -O2



// sum a list of the same size
// c++                                 0.57 sec
// prototype of new memory model       1.89 sec
// running time with improved model    