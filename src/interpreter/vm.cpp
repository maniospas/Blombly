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
        optimize(fileName + "vm", fileName + "vm", true); // always minify
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

    return Result(new Code(program, 0, program->size() - 1));
}

extern std::unordered_map<int, Data*> cachedData;

int vm(const std::string& fileName, int numThreads) {
    Future::setMaxThreads(numThreads);
    bool hadError = false;
    try {
        {
            std::ifstream inputFile(fileName);
            bbassert(inputFile.is_open(), "Unable to open file: " + fileName);

            auto program = new std::vector<Command*>();
            auto source = new SourceFile(fileName);
            std::string line;
            int i = 1;
            
            CommandContext* descriptor = nullptr;
            while (std::getline(inputFile, line)) {
                if (line[0] != '%') program->push_back(new Command(line, source, i, descriptor));
                else descriptor = new CommandContext(line.substr(1));
                ++i;
            }
            inputFile.close();
            
            BMemory memory(nullptr, DEFAULT_LOCAL_EXPECTATION);
            try {
                auto code = new Code(program, 0, program->size() - 1);
                bool hasReturned(false);
                executeBlock(code, &memory, hasReturned, false);
                bbassert(!hasReturned, "The virtual machine cannot return a value.");
                //memory.detach(nullptr);
            }
            catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        for (const auto& [key, data] : cachedData) data->removeFromOwner();
        cachedData.clear();
        BMemory::verify_noleaks();
        //std::cout<<"Program completed successfully\n";
    } catch (const BBError& e) {
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }
    if(hadError) {
        std::cerr << "Docs and bug reports: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

extern std::string compileFromCode(const std::string& code, const std::string& source);
extern std::string optimizeFromCode(const std::string& code, bool minimify);

int vmFromSourceCode(const std::string& sourceCode, int numThreads) {
    Future::setMaxThreads(numThreads);
    bool hadError = false;
    try {
        {
            std::string newCode = compileFromCode(sourceCode, "terminal argument");
            newCode = optimizeFromCode(newCode, true); 
            BMemory memory(nullptr, DEFAULT_LOCAL_EXPECTATION);
            try {
                std::istringstream inputFile(newCode);

                auto program = new std::vector<Command*>();
                auto source = new SourceFile("terminal argument");
                std::string line;
                int i = 1;
                
                CommandContext* descriptor = nullptr;
                while (std::getline(inputFile, line)) {
                    if (line[0] != '%') program->push_back(new Command(line, source, i, descriptor));
                    else descriptor = new CommandContext(line.substr(1));
                    ++i;
                }

                auto code = new Code(program, 0, program->size() - 1);
                bool hasReturned(false);
                if(numThreads) executeBlock(code, &memory, hasReturned, false);
                bbassert(!hasReturned, "The virtual machine cannot return a value.");
                //memory.detach(nullptr);
            }
            catch (const BBError& e) {
                std::cerr << e.what() << "\033[0m\n";
                hadError = true;
            }
            memory.release();
        }
        for (const auto& [key, data] : cachedData) data->removeFromOwner();
        cachedData.clear();
        BMemory::verify_noleaks();
        //std::cout<<"Program completed successfully\n";
    } catch (const BBError& e) {
        std::cerr << e.what() << "\033[0m\n";
        hadError = true;
    }
    if(hadError) {
        std::cerr << "Docs and bug reports: https://maniospas.github.io/Blombly\n";
        return 1;
    }
    return 0;
}

#endif