// Future.cpp
#include "data/Future.h"
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
Future::Future(const std::shared_ptr<FutureData>& data_) : data(std::move(data_)) {}

// Return the data type
int Future::getType() const {
    return FUTURE;
}

// Convert to string representation
std::string Future::toString() const {
    return "future";
}

// Create a shallow copy of this Future
Data* Future::shallowCopy() const {
    bberror("Internal error: threads can not be copied");
    return getResult()->shallowCopy();
    //return new Future(data);
}

// Get the result after joining the thread
Data* Future::getResult() const {
    try {
        if (data->thread.joinable()) 
            data->thread.join();
    } catch (...) {
        bberror("Failed to join thread");
    }
    auto res = data->result;
    if(res->error)
        throw *res->error;
    return res->value;
}
