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
extern double wallclock_start;
extern std::recursive_mutex printMutex;
extern std::recursive_mutex compileMutex;

struct ExecutionInstanceRunReturn {
    bool returnSignal;
    Result result;

    ExecutionInstanceRunReturn() = delete;
    explicit ExecutionInstanceRunReturn(bool returnSignal, Result result) : returnSignal(returnSignal), result(std::move(result)) {}
    ExecutionInstanceRunReturn(const ExecutionInstanceRunReturn& other) = delete;
    ExecutionInstanceRunReturn(ExecutionInstanceRunReturn&& other) noexcept : returnSignal(other.returnSignal), result(std::move(other.result)) {}
    ExecutionInstanceRunReturn& operator=(const ExecutionInstanceRunReturn& other) { returnSignal = other.returnSignal; result = other.result; return *this; }
    ExecutionInstanceRunReturn& operator=(ExecutionInstanceRunReturn&& other) noexcept { returnSignal = other.returnSignal; result = std::move(other.result); return *this; }
    ~ExecutionInstanceRunReturn() = default;
    inline const DataPtr& get() { return result.get(); }
};


class ExecutionInstance {
    BMemory& memory;
    bool forceStayInThread;
    unsigned int depth;
    DataPtr result;
public:
    static unsigned int maxDepth;
    ExecutionInstance(int depth, Code* code, BMemory* memory, bool forceStayInThread): 
        result(DataPtr::NULLP), memory(*memory),  forceStayInThread(forceStayInThread), depth(depth+1) {
        if(depth>=maxDepth) bberrorexplain("Maximum call stack depth reached: "+std::to_string(depth), "The call stack includes inline redirections of control flow. Reaching maximal depth typically indicates a logical error, such as unbounded recursion. If you are sure your code is correct, run blombly with greater --depth.", "");
    }
    ExecutionInstanceRunReturn run(Code* code);
    ExecutionInstanceRunReturn run(const std::vector<Command>& program, size_t i, size_t end);
    void handleExecutionError(const Command& command, const BBError& e);
};


std::string enrichErrorDescription(const Command&, std::string message);
std::string getStackFrame(const Command& command);
int vm(const std::string& fileName, int numThreads);
int vmFromSourceCode(const std::string& sourceCode, int numThreads);
std::string __python_like_float_format(double number, const std::string& format);
std::string __python_like_int_format(int64_t int_number, const std::string& format);

#endif