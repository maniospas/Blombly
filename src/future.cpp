// Future.cpp
#include "Future.h"
#include "common.h"
#include <iostream>

// FutureData constructor
FutureData::FutureData() : result(std::make_shared<ThreadResult>()) {}

// FutureData destructor
FutureData::~FutureData() {
    if (thread.joinable()) {
        thread.join();
    }
}

// Future constructor
Future::Future(std::shared_ptr<FutureData> data) : data(std::move(data)) {}

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
    return std::make_shared<Future>(data);
}

// Get the result after joining the thread
std::shared_ptr<Data> Future::getResult() {
    if (data->thread.joinable()) {
        try {
            data->thread.join();
        } catch (...) {
            std::cerr << "Failed to join thread\n";
            exit(1);
        }
    }
    return data->result->value;
}
