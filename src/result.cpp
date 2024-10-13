#include "Result.h"
#include "data/Data.h" // Assuming Data is implemented elsewhere


Result::Result(Data* data) : data(data) {
    if (data) 
        data->addOwner();
}

Result::Result(const Result& other) : data(other.data) {
    if (data) 
        data->addOwner();
}

Result::Result(Result& other) : data(other.data) {
    if (data) 
        data->addOwner();
}

Result::Result(Result&& other) noexcept : data(other.data) {
    other.data = nullptr;
}

Result::~Result() {
    if (data) 
        data->removeFromOwner();
}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        if(data)
            data->removeFromOwner();
        data = other.data;
        if (data) 
            data->addOwner();
    }
    return *this;
}

Result& Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        if(data)
            data->removeFromOwner();
        data = other.data;
        other.data = nullptr;
    }
    return *this;
}

Data* Result::get() const {
    return data;
}
