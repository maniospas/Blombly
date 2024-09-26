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

std::shared_ptr<Data> executeBlock(const std::shared_ptr<Code>& code, const std::shared_ptr<BMemory>& memory, bool  &returnSignal, const BuiltinArgs& allocatedBuiltins);
void handleExecutionError(const std::shared_ptr<std::vector<Command*>>& program, int i, const BBError& e);
void handleCommand(const std::shared_ptr<std::vector<Command*>>& program, int& i, const std::shared_ptr<BMemory>& memory, bool &returnSignal, BuiltinArgs &args, std::shared_ptr<Data>& result);

std::shared_ptr<Code> compileAndLoad(const std::string& fileName, const std::shared_ptr<BMemory>& currentMemory);
int vm(const std::string& fileName, int numThreads);

#endif