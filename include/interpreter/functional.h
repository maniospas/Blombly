#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <iostream>
#include "common.h"
#include "BMemory.h"
#include "data/Code.h"
#include "interpreter/Command.h"
#include "Result.h"

extern std::chrono::steady_clock::time_point program_start;
extern std::recursive_mutex printMutex;
extern std::recursive_mutex compileMutex;

class ExecutionInstance {
    BuiltinArgs args;
    DataPtr result;
    std::vector<Command>& program;
    BMemory& memory;
    bool returnSignal;
    bool forceStayInThread;
    void handleCommand(int &i);
public:
    ExecutionInstance(Code* code, BMemory* memory, bool forceStayInThread): returnSignal(false), program(*code->getProgram()), memory(*memory), forceStayInThread(forceStayInThread) {}
    Result run(Code* code);
    void handleExecutionError(int i, const BBError& e);
    inline bool hasReturned() const {return returnSignal;}
};

std::string enrichErrorDescription(const Command&, std::string message);
Result compileAndLoad(const std::string& fileName, BMemory* currentMemory);
int vm(const std::string& fileName, int numThreads);
int vmFromSourceCode(const std::string& sourceCode, int numThreads);

#endif