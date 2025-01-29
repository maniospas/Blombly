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

#include "data/Future.h"
#include "common.h"
#include <iostream>
#include <atomic>

int Future::max_threads = 0;
std::atomic<int> Future::thread_count = 0;

Future::Future() : result(new ThreadResult()), Data(FUTURE) {
    ++thread_count;
}

Future::Future(ThreadResult* result_) : result((result_)), Data(FUTURE) {
    ++thread_count;
}

// Future destructor
Future::~Future() {
    {
        if (result->thread.joinable()) {
            result->thread.join();
            --thread_count;
        }
    }
    delete result;
}

// Check if a new thread can be accepted
bool Future::acceptsThread() {
    return thread_count < max_threads;
}

// Set maximum threads
void Future::setMaxThreads(int maxThreads_) {
    max_threads = maxThreads_;
}

// Convert to string representation
std::string Future::toString(BMemory* memory){
    return "future";
}

// Get the result after joining the thread
Result Future::getResult() const {
    try { 
        if (result->thread.joinable()) {
            result->thread.join();
            --thread_count;
        }
    } catch (...) {
        result->error = nullptr;
        result->value = Result(DataPtr::NULLP);
        --thread_count;
        bberror("Failed to join thread");
    }
    if (result->error) {
        std::string error_message = RESMOVE(result->error->what());
        result->error = nullptr;
        result->value = Result(DataPtr::NULLP);
        throw BBError(error_message);
    }
    DataPtr ret = result->value.get();
    if(ret.existsAndTypeEquals(FUTURE)) return RESMOVE(static_cast<Future*>(ret.get())->getResult());
    return RESMOVE(Result(ret));
}
