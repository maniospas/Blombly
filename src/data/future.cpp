#include "data/Future.h"
#include "common.h"
#include <iostream>

int Future::max_threads = 0;
int Future::thread_count = 0;

// Future constructor
Future::Future() : result(std::make_shared<ThreadResult>()) {}

// Constructor with thread and result
Future::Future(std::shared_ptr<ThreadResult> result_)
    : result(std::move(result_)) {
    ++thread_count;
}

// Future destructor
Future::~Future() {
    if (result->thread.joinable()) 
        result->thread.join();
}

// Check if a new thread can be accepted
bool Future::acceptsThread() {
    return thread_count < max_threads;
}

// Set maximum threads
void Future::setMaxThreads(int maxThreads) {
    max_threads = maxThreads;
}

// Return the data type
int Future::getType() const {
    return FUTURE;
}

// Convert to string representation
std::string Future::toString() const {
    return "future";
}

// Create a shallow copy of this Future
std::shared_ptr<Data> Future::shallowCopy() const {
    bberror("Internal error: threads cannot be copied");
    auto res = getResult();
    SCOPY(res);
    return res;
}

// Get the result after joining the thread
std::shared_ptr<Data> Future::getResult() const {
    try {
        if (result->thread.joinable()) {
            result->thread.join();
        }
    } catch (...) {
        bberror("Failed to join thread");
    }

    --thread_count;

    if (result->error) {
        std::string error_message = std::move(result->error->what());
        result->error = nullptr;
        result->value = nullptr;
        bberror(error_message);
    }

    return result->value;
}
