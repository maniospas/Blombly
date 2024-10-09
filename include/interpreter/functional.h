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

extern std::chrono::steady_clock::time_point program_start;
extern std::recursive_mutex printMutex;
extern std::recursive_mutex compileMutex;

Data* executeBlock(Code* code, BMemory* memory, bool &returnSignal);
void handleExecutionError(std::vector<Command*>* program, int i, const BBError& e);
void handleCommand(std::vector<Command*>* program, int& i, BMemory* memory, bool &returnSignal, BuiltinArgs &args, Data*& result);

Code* compileAndLoad(const std::string& fileName, BMemory* currentMemory);
int vm(const std::string& fileName, int numThreads);

#endif