#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include "BMemory.h"
#include "data/Code.h"
#include "data/Future.h"
#include "data/BError.h"
#include "interpreter/Command.h"
#include "utils.h"
#include "interpreter/functional.h"

// git clone https://github.com/microsoft/vcpkg.git

// Experimental cmake build system (other compilers may be up to 1.5x slower than gcc):
// mkdir build
// cmake -B ./build
// FOR LOCAL ENV:  cmake -B ./build -DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe -DCMAKE_CXX_COMPILER=C:\msys64\ucrt64\bin\gcc.exe
// cmake --build ./build --config Release

std::string blombly_executable_path;

BError* OUT_OF_RANGE = new BError("Out of range");
BError* INCOMPATIBLE_SIZES = new BError("Incompatible sizes");


std::string get_executable_directory(const std::string& argv0) {
    #ifdef _WIN32
        size_t pos = argv0.find_last_of("\\");
    #else
        size_t pos = argv0.find_last_of("/");
    #endif
    if (pos != std::string::npos) 
        return argv0.substr(0, pos);
    return ".";
}

int main(int argc, char* argv[]) {
    OUT_OF_RANGE->consume();
    INCOMPATIBLE_SIZES->consume();
    OUT_OF_RANGE->addOwner();
    INCOMPATIBLE_SIZES->addOwner();
    
    blombly_executable_path = get_executable_directory(argv[0]);
    Terminal::enableVirtualTerminalProcessing();
    initializeOperationMapping();   
    std::cout<<"\033[0m";

    std::string fileName = "main.bb";
    int threads = std::thread::hardware_concurrency();
    int default_threads = threads;
    bool cexecute = false;

    if (threads == 0) 
        threads = 4;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--threads" || arg == "-threads") && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } 
        else if (arg == "--version" || arg == "-v") {
            std::cout << "Version: blombly 1.3.0\n";
            return 0;
        } 
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: blombly [options] [file]\n";
            std::cout << "--threads <num>   Set max threads. Default for this machine: " << default_threads << "\n";
            return 0;
        } 
        else {
            fileName = arg;
        }
    }

    try {
        if (fileName.substr(fileName.size() - 3, 3) == ".bb") {
            compile(fileName, fileName + "vm");
            optimize(fileName + "vm", fileName + "vm");
            fileName = fileName + "vm";
        }
    }
    catch (const BBError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    
    program_start = std::chrono::steady_clock::now();
    auto ret = vm(fileName, threads);

    OUT_OF_RANGE->removeFromOwner();
    INCOMPATIBLE_SIZES->removeFromOwner();

    return ret;
}
