#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include "BMemory.h"
#include "data/Code.h"
#include "data/Future.h"
#include "interpreter/Command.h"
#include "utils.h"
#include "interpreter/functional.h"


std::chrono::steady_clock::time_point program_start;
pthread_mutex_t printLock;
pthread_mutex_t compileLock;


int main(int argc, char* argv[]) {
    Terminal::enableVirtualTerminalProcessing();
    initializeOperationMapping();

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
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "Version: blombly 0.2.1\n";
            return 0;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: blombly [options] [file]\n";
            std::cout << "--threads <num>   Set max threads. Default: " << default_threads << "\n";
            return 0;
        } else {
            fileName = arg;
        }
    }

    if (fileName.substr(fileName.size() - 3, 3) == ".bb") {
        try {
            compile(fileName, fileName + "vm");
        } catch (const BBError& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        try {
            optimize(fileName + "vm", fileName + "vm");
        } catch (const BBError& e) {
            std::cerr << e.what() << "\n";
            return 1;
        }

        fileName = fileName + "vm";
    }

    if (pthread_mutex_init(&printLock, nullptr) != 0) {
        std::cerr << "Print mutex initialization failed.\n";
        return 1;
    }

    if (pthread_mutex_init(&compileLock, nullptr) != 0) {
        std::cerr << "Compile mutex initialization failed.\n";
        return 1;
    }

    program_start = std::chrono::steady_clock::now();
    return vm(fileName, threads);
}
