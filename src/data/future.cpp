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
    if (result->thread.joinable()) 
        result->thread.join();
    delete result;
}

// Check if a new thread can be accepted
bool Future::acceptsThread() {
    return thread_count < max_threads;
}

// Set maximum threads
void Future::setMaxThreads(int maxThreads) {
    max_threads = maxThreads;
}

// Convert to string representation
std::string Future::toString() const {
    return "future";
}

// Get the result after joining the thread
Result Future::getResult() const {
    std::lock_guard<std::recursive_mutex> lock(syncMutex);
    try {
        if (result->thread.joinable()) {
            result->thread.join();
            --thread_count;
        }
    } catch (...) {
        bberror("Failed to join thread");
    }

    if (result->error) {
        std::string error_message = (result->error->what());
        result->error = nullptr;
        result->value = Result(nullptr);
        throw BBError(error_message);
    }
    return std::move(Result(result->value.get()));
}
