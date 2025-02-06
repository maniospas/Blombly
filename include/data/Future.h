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

#ifndef FUTURE_H
#define FUTURE_H

#include <memory>
#include <thread>
#include "data/Data.h"
#include "data/BError.h"
#include "interpreter/Command.h"
#include "interpreter/thread.h"
#include "BMemory.h"
#include "data/Code.h"


class ThreadResult {
public:
    std::thread thread;
    Result value;
    ThreadResult():value(DataPtr::NULLP) {};
    ~ThreadResult() = default;
    void start(unsigned int depth, Code* code, BMemory* newMemory, ThreadResult* futureResult, const Command* command, DataPtr thisObj) {
        thread = std::thread(threadExecute, depth, code, newMemory, futureResult, command, thisObj);
    }
};


class Future : public Data {
private:
    ThreadResult* result;
    static std::atomic<int> thread_count;
    static int max_threads;

public:
    Future();
    explicit Future(ThreadResult* result);
    ~Future();

    std::string toString(BMemory* memory)override;
    Result getResult() const;

    static bool acceptsThread();
    static void setMaxThreads(int maxThreads);
};

#endif // FUTURE_H
