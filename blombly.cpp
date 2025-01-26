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
// codespell docs/ -i 3 -w

// Experimental cmake build system (other compilers may be up to 1.5x slower than gcc):
// mkdir build
// cmake -B ./build
// FOR LOCAL ENV:  cmake -G "MinGW Makefiles" -B ./build
// cmake --build ./build --config Release --parallel 8

std::string blombly_executable_path;
extern std::string top_level_file;
bool debug_info = true;
BError* OUT_OF_RANGE = new BError("Out of range");
BError* INCOMPATIBLE_SIZES = new BError("Incompatible sizes in operation");
BError* NO_TRY_INTERCEPT = new BError("No error or return statement intercepted with `try`.");
extern void clearAllowedLocations();
extern void initialize_dispatch_table();

std::string get_executable_directory(const std::string& argv0) {
    #ifdef _WIN32
        size_t pos = argv0.find_last_of("\\");
    #else
        size_t pos = argv0.find_last_of("/");
    #endif
    if (pos != std::string::npos) return argv0.substr(0, pos);
    return ".";
}

int main(int argc, char* argv[]) {
    initialize_dispatch_table();
    OUT_OF_RANGE->consume();
    OUT_OF_RANGE->addOwner();
    INCOMPATIBLE_SIZES->consume();
    INCOMPATIBLE_SIZES->addOwner();
    NO_TRY_INTERCEPT->consume();
    NO_TRY_INTERCEPT->addOwner();
    
    blombly_executable_path = get_executable_directory(argv[0]);
    Terminal::enableVirtualTerminalProcessing();
    initializeOperationMapping();  
    clearAllowedLocations(); 
    std::cout<<"\033[0m";

    int threads = std::thread::hardware_concurrency();
    int default_threads = threads;
    bool minimify = true;

    if(threads == 0) threads = 4;
    std::vector<std::string> instructions;
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if((arg == "--threads" || arg == "-t") && i + 1 < argc) threads = std::stoi(argv[++i]);
        else if(arg == "--version" || arg == "-v") {
            std::cout << "Version: blombly 1.19.0\n";
            return 0;
        } 
        else if(arg == "--help" || arg == "-h") {
            std::cout << "Usage: ./blombly [file] [options] \n";
            std::cout << "--threads <num>   Set max threads. Default for this machine: " << default_threads << "\n";
            std::cout << "--norun           Compiles a bbvm file only by setting max threads to zero.\n";
            std::cout << "--library         Prevents compilation optimizations\n";
            std::cout << "--strip           Strips away debugging symbols\n";
            std::cout << "--version         Prints the current blombly version\n";
            return 0;
        } 
        else if(arg == "--library" || arg == "-l") minimify = false;
        else if(arg == "--strip" || arg == "-s") debug_info = false;
        else if(arg == "--norun") threads = 0;
        else instructions.push_back(arg);
    }

    if(instructions.empty()) instructions.push_back("main.bb");

    int ret = 0;
    for(std::string fileName : instructions) {
        top_level_file = "";
        if(!(fileName.size()>=3 && fileName.substr(fileName.size() - 3, 3) == ".bb")
            && !(fileName.size()>=3 && fileName.substr(fileName.size() - 3, 3) == ".bb")) {
            ret = vmFromSourceCode(fileName, threads);
            if(ret) break;
            continue;
        }
        try {
            bbassert((fileName.size()>=3 && fileName.substr(fileName.size() - 3, 3) == ".bb")
                        || (fileName.size()>=3 && fileName.substr(fileName.size() - 3, 3) == ".bb"),
                        "Blombly can only compile and run .bb or .bbvm files, or code enclosed in single quotes '...', but an invalid option was provided: "+fileName)
            if(fileName.size()>=3 && fileName.substr(fileName.size() - 3, 3) == ".bb") {
                compile(fileName, fileName + "vm");
                optimize(fileName + "vm", fileName + "vm", minimify);
                fileName = fileName + "vm";
            }
        }
        catch(const BBError& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
        
        if(threads) {
            program_start = std::chrono::steady_clock::now();
            ret = vm(fileName, threads);
        }
        if(ret) break;
    }

    OUT_OF_RANGE->removeFromOwner();
    INCOMPATIBLE_SIZES->removeFromOwner();
    NO_TRY_INTERCEPT->removeFromOwner();

    return ret;
}
