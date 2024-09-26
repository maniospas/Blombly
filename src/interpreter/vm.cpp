#ifndef VM_CPP
#define VM_CPP

#include <memory>
#include <mutex>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "BMemory.h"
#include "data/Future.h"
#include "utils.h"
#include "interpreter/functional.h"


extern std::chrono::steady_clock::time_point program_start;
extern pthread_mutex_t printLock;
extern pthread_mutex_t compileLock;


std::shared_ptr<Code> compileAndLoad(const std::string& fileName, const std::shared_ptr<BMemory>& currentMemory) {
    pthread_mutex_lock(&compileLock);

    // Compile and optimize
    std::string file = fileName;
    if (fileName.substr(fileName.size() - 3, 3) == ".bb") {
        compile(fileName, fileName + "vm");
        optimize(fileName + "vm", fileName + "vm");
        file = fileName + "vm";
    }

    // Open the compiled .bbvm file
    std::ifstream inputFile(file);
    if (!inputFile.is_open()) {
        bberror("Unable to open file: " + file);
        return nullptr;
    }

    // Organize each line into a new assembly command
    auto program = std::make_shared<std::vector<Command>>();
    auto source = std::make_shared<SourceFile>(file);
    std::string line;
    int i = 1;
    while (std::getline(inputFile, line)) {
        if (line[0] != '%') {
            program->emplace_back(line, source, i, nullptr);
        }
        ++i;
    }
    inputFile.close();

    pthread_mutex_unlock(&compileLock);

    return std::make_shared<Code>(program, 0, program->size() - 1, currentMemory);
}


int vm(const std::string& fileName, int numThreads) {
    Future::setMaxThreads(numThreads);

    try {
        std::ifstream inputFile(fileName);
        if (!inputFile.is_open()) {
            bberror("Unable to open file: " + fileName);
        }

        auto program = std::make_shared<std::vector<Command>>();
        auto source = std::make_shared<SourceFile>(fileName);
        std::string line;
        int i = 1;

        while (std::getline(inputFile, line)) {
            if (line[0] != '%') {
                program->emplace_back(line, source, i, nullptr);
            }
            ++i;
        }

        inputFile.close();

        auto memory = std::make_shared<BMemory>(nullptr, DEFAULT_LOCAL_EXPECTATION);
        auto code = std::make_shared<Code>(program, 0, program->size() - 1, memory);
        bool hasReturned(false);
        executeBlock(code, memory, hasReturned, BuiltinArgs());
        bbassert(!hasReturned, "The virtual machine problem cannot return a value.");

        BMemory::verify_noleaks();
    } catch (const BBError& e) {
        std::cerr << e.what() << "\n";
        std::cerr << "Docs and bug reports: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

#endif