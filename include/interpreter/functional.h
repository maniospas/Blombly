/*
   Copyright 2024 Emmanouil Krasanakis

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

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
    BMemory& memory;
    bool returnSignal;
    bool forceStayInThread;
    Code* lastCall;
    unsigned int depth;
    DataPtr result;
    DataPtr arg0, arg1;
public:
    static unsigned int maxDepth;
    ExecutionInstance(int depth, Code* code, BMemory* memory, bool forceStayInThread): 
        result(DataPtr::NULLP), memory(*memory), returnSignal(false), forceStayInThread(forceStayInThread), depth(depth+1) {
        if(depth>=maxDepth) bberror("Maximum call stack depth reached: "+std::to_string(depth)+"\nThis typically indicates a logical error. If not, run with greater --depth.");
    }
    Result run(Code* code);
    Result run(const std::vector<Command>& program, size_t i, size_t end);
    void handleExecutionError(const Command& command, const BBError& e);
    inline bool hasReturned() const {return returnSignal;}
};


std::string enrichErrorDescription(const Command&, std::string message);
int vm(const std::string& fileName, int numThreads);
int vmFromSourceCode(const std::string& sourceCode, int numThreads);
std::string __python_like_float_format(double number, const std::string& format);
std::string __python_like_int_format(int64_t int_number, const std::string& format);

#endif