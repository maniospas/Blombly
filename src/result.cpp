#include "Result.h"
#include "data/Data.h" // Assuming Data is implemented elsewhere


Result::Result(DataPtr data) : data(data) {
    if (data.exists()) data->addOwner();
}

Result::Result(Result& other) noexcept : data(other.data) {
    //other.data = nullptr;
    if (data.exists()) data->addOwner();
}

Result::Result(Result&& other) noexcept : data(other.data) {
    other.data = nullptr;
}

Result::~Result() {
    if (data.exists()) data->removeFromOwner();
}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        data = other.data;
        if (data.exists()) data->addOwner();
    }
    return *this;
}

Result& Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        if(data.exists()) data->removeFromOwner();
        data = other.data;
        other.data = nullptr;
    }
    return *this;
}

DataPtr Result::get() const {
    return data;
}
