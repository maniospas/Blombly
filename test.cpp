#include <iostream>
#include "include/tsl/hopscotch_map.h"
#include <chrono>

int main() {
    tsl::hopscotch_map<int, int> map;
    map[1] = 1;
    map[2] = 1;
    map[5] = 1;
    map[76] = 1;
    int n=10000000;
    {
        std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

        int x;
        for(int i=0;i<n;i++) {
            x = map[76];
        }

        std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
        std::cout<<std::chrono::duration_cast<std::chrono::duration<double>>(toc-tic).count()<<" sec\n";
    }
    {
        std::chrono::steady_clock::time_point tic = std::chrono::steady_clock::now();

        int x;
        for(int i=0;i<n;i++) {
            auto it = map.find(76);
            if(it!=map.end())
                x = it->second;
        }

        std::chrono::steady_clock::time_point toc = std::chrono::steady_clock::now();
        std::cout<<std::chrono::duration_cast<std::chrono::duration<double>>(toc-tic).count()<<" sec\n";
    }

}