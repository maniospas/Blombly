#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <memory>
#include <string>
#include <iostream>
#include "common.h"
#include "BMemory.h"
#include "data/Code.h"
#include "interpreter/Command.h"

std::shared_ptr<Data> executeBlock(const std::shared_ptr<Code>& code, const std::shared_ptr<BMemory>& memory, bool  &returnSignal, const BuiltinArgs& allocatedBuiltins);
void handleExecutionError(const std::shared_ptr<std::vector<Command>>& program, int i, const BBError& e);
std::shared_ptr<Data> handleCommand(const std::shared_ptr<std::vector<Command>>& program, int& i, const std::shared_ptr<BMemory>& memory, bool &returnSignal, BuiltinArgs &args);

std::shared_ptr<Code> compileAndLoad(const std::string& fileName, const std::shared_ptr<BMemory>& currentMemory);
int vm(const std::string& fileName, int numThreads);

#endif