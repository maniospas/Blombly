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
        result->value = Result(nullptr);
        --thread_count;
        bberror("Failed to join thread");
    }
    if (result->error) {
        std::string error_message = RESMOVE(result->error->what());
        result->error = nullptr;
        result->value = Result(nullptr);
        throw BBError(error_message);
    }
    DataPtr ret = result->value.get();
    if(ret.existsAndTypeEquals(FUTURE)) return std::move(static_cast<Future*>(ret.get())->getResult());
    //std::cout << result->value.get() << "\n";
    return RESMOVE(Result(ret));
}
