#include <fstream>  // Required for std::ofstream
#include <iostream> // Required for std::cerr and std::cout
#include <cstdlib>  // Required for system()
#include <dlfcn.h>  // Required for dlopen and dlsym
#include <cassert>  // Required for assert

// Compilation: g++ -o testjit testjit.cpp -ldl
void bbassert(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << "\n";
        std::exit(EXIT_FAILURE);
    }
}

class Compile {
    void *handle;
    void *func;
public:
    Compile(std::string code, std::string name) {
        std::string filename = "temp.jit.bb";
        std::ofstream(filename+".c") << code;
        int ret = system(("gcc -shared -fPIC ./"+filename+".c -o ./"+filename+".so -O2").c_str());
        bbassert(ret == 0, "Compilation failed");
        handle = dlopen(("./"+filename+".so").c_str(), RTLD_LAZY);
        bbassert(handle != nullptr, dlerror());
        bbassert(std::remove(("./"+filename+".so").c_str())==0, "Failed to remove a temporary file");
        bbassert(std::remove(("./"+filename+".c").c_str())==0, "Failed to remove a temporary file");
        func = dlsym(handle, "add");
        bbassert(func != nullptr, dlerror());
    }
    ~Compile() {dlclose(handle);}
    void* get() {return func;}
};


int main() {
    std::string code = R"(
        #include <stdio.h>
        int add(int a, int b) {
            return a + b;
        }
    )";

    Compile compile(code, "add");
    auto add = (int (*)(int, int))compile.get();
    std::cout << "Result: " << add(10, 20) << "\n";

    return 0;
}
