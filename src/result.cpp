#include "Result.h"
#include "data/Data.h" // Assuming Data is implemented elsewhere


Result::Result(DataPtr data) noexcept : data(data)  {
    data.existsAddOwner();
}

//Result::Result(const DataPtr& data) noexcept : data(data) {
//    if (data.exists()) data->addOwner();
//}

Result::Result(Result& other) noexcept : data(other.data) {
    //other.data = nullptr;
    data.existsAddOwner();
}

Result::Result(Result&& other) noexcept : data(std::move(other.data)) {
    other.data = DataPtr::NULLP;
}

Result::~Result() {
    data.existsRemoveFromOwner();
}

Result& Result::operator=(const Result& other) {
    if (this != &other) {
        auto prevData = data;
        data = other.data;
        data.existsAddOwner();
        prevData.existsRemoveFromOwner();
    }
    return *this;
}

Result& Result::operator=(Result&& other) noexcept {
    if (this != &other) {
        data.existsRemoveFromOwner();
        data = std::move(other.data);
        other.data = DataPtr::NULLP;
    }
    return *this;
}

const DataPtr& Result::get() const {return data;}
