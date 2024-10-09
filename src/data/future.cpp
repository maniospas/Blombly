#include "data/Future.h"
#include "common.h"
#include <iostream>

int Future::max_threads = 0;
int Future::thread_count = 0;

// Future constructor
Future::Future() : result(new ThreadResult()) {}

// Constructor with thread and result
Future::Future(ThreadResult* result_) : result((result_)) {
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

// Return the data type
int Future::getType() const {
    return FUTURE;
}

// Convert to string representation
std::string Future::toString() const {
    return "future";
}

// Get the result after joining the thread
Data* Future::getResult() const {
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
        result->value = nullptr;
        throw BBError(error_message);
    }

    return result->value;
}
