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

Result executeBlock(Code* code, BMemory* memory, bool &returnSignal, bool forceStayInThread);
std::string enrichErrorDescription(Command*, std::string message);
void handleExecutionError(std::vector<Command*>* program, int i, const BBError& e);
void handleCommand(std::vector<Command*>* program, int& i, BMemory* memory, bool &returnSignal, BuiltinArgs &args, Data*& result, bool forceStayInThread);

Result compileAndLoad(const std::string& fileName, BMemory* currentMemory);
int vm(const std::string& fileName, int numThreads);

#endif