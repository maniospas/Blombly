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


Result compileAndLoad(const std::string& fileName, BMemory* currentMemory) {
    std::lock_guard<std::recursive_mutex> lock(compileMutex);

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
        return Result(nullptr);
    }

    // Organize each line into a new assembly command
    auto program = new std::vector<Command*>();
    auto source = new SourceFile(file);
    std::string line;
    int i = 1;
    while (std::getline(inputFile, line)) {
        if (line[0] != '%') {
            program->push_back(new Command(line, source, i, nullptr));
        }
        ++i;
    }
    inputFile.close();

    return Result(new Code(program, 0, program->size() - 1, currentMemory));
}


int vm(const std::string& fileName, int numThreads) {
    Future::setMaxThreads(numThreads);

    try {
        {
            BMemory memory(nullptr, DEFAULT_LOCAL_EXPECTATION);
            std::ifstream inputFile(fileName);
            if (!inputFile.is_open()) {
                bberror("Unable to open file: " + fileName);
            }

            auto program = new std::vector<Command*>();
            auto source = new SourceFile(fileName);
            std::string line;
            int i = 1;

            while (std::getline(inputFile, line)) {
                if (line[0] != '%') {
                    program->push_back(new Command(line, source, i, nullptr));
                }
                ++i;
            }

            inputFile.close();

            auto code = new Code(program, 0, program->size() - 1, &memory);
            bool hasReturned(false);
            executeBlock(code, &memory, hasReturned);
            bbassert(!hasReturned, "The virtual machine cannot return a value.");
        }
        BMemory::verify_noleaks();
        //std::cout<<"Program completed successfully\n";
    } catch (const BBError& e) {
        std::cerr << e.what() << "\033[0m\n";
        std::cerr << "Docs and bug reports: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

#endif