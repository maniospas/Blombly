#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>


int main(int argc, char* argv[]) {
    int n = 10000000;
    double* a = new double[n];
    double* b = new double[n];
    for(int i=0;i<n;i++) {
        a[i] = 0;
        b[i] = 0;
    }
    std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

    double* c = new double[n];
    for(int i=0;i<n;i++) {
        c[i] = a[i]+b[i];
    }
    std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
    std::cout<<std::chrono::duration_cast<std::chrono::duration<double>>(toc-tic).count()<<" sec\n";
}