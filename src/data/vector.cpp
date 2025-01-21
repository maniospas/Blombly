#include "data/Vector.h"
#include "data/BString.h"
#include "common.h"
#include "data/Iterator.h"
#include "data/BError.h"
#include <iostream>
#include <cmath>

extern BError* OUT_OF_RANGE;
extern BError* INCOMPATIBLE_SIZES;

Vector::Vector(uint64_t size) : data(new double[size]), size(size), Data(VECTOR) {}
Vector::Vector(uint64_t size, bool setToZero) : Vector(size) {if (setToZero) std::fill(data, data + size, 0);}
Vector::~Vector() {delete[] data;}

std::string Vector::toString(BMemory* memory){
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    std::string result("");
    for (std::size_t i = 0; i < std::min(static_cast<std::size_t>(size), static_cast<std::size_t>(10)); ++i) {
        if (result.size() > 1) result += ", ";
        result += std::to_string(data[i]);
    }
    if (size > 10) result += ", ...";
    return result;
}


Result Vector::at(BMemory* memory, const DataPtr& other) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (other.isint()) {
        int64_t index = other.unsafe_toint();
        if (index < 0 || index >= size) return Result(OUT_OF_RANGE);
        return std::move(Result(data[index]));
    }

    if (other.existsAndTypeEquals(LIST) || other.existsAndTypeEquals(ITERATOR)) {
        Result iter = other->iter(memory);
        const DataPtr& iterator = iter.get();
        bbassert(iterator.existsAndTypeEquals(ITERATOR), "Can only find vector indexes based on an iterable object, but a non-iterable struct was provided.");

        Iterator* iterPtr = static_cast<Iterator*>(iterator.get());
        if (iterPtr->isContiguous()) {
            int64_t start = iterPtr->getStart();
            int64_t end = iterPtr->getEnd();
            if (start < 0 || end < 0 || start >= size || end > size) return Result(OUT_OF_RANGE);
            auto* resultVec = new Vector(end - start);
            for (int64_t i = start; i < end; ++i) resultVec->data[i - start] = data[i];
            return Result(resultVec);
        } 
        else {
            auto* resultVec = new Vector(iterPtr->expectedSize());
            int indexCount = 0;
            while (true) {
                Result next = iterator->next(memory);
                DataPtr indexData = next.get();
                if (!indexData.exists() || indexData.get() == OUT_OF_RANGE) break;
                bbassert(indexData.isint(), "Iterable vector indexes can only contain integers.");
                int64_t idx = indexData.unsafe_toint();
                if (idx < 0 || idx >= size) return Result(OUT_OF_RANGE);
                resultVec->data[indexCount++] = data[idx];
            }
            return Result(resultVec);
        }
    }

    return Result(OUT_OF_RANGE);
}

Result Vector::put(BMemory* memory, const DataPtr& position, const DataPtr& value) {
    double val;
    if (value.isfloat()) val = value.unsafe_tofloat();
    else if (value.isint()) val = value.unsafe_toint();
    else bberror("No builtin implementation for put(vector, "+value.torepr()+")");

    int64_t index = position.unsafe_toint();
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (index < 0 || index >= size) return Result(OUT_OF_RANGE);
    data[index] = val;
    return Result(nullptr);
}

int64_t Vector::len(BMemory* memory) {return size;}
Result Vector::iter(BMemory* memory) {return Result(new AccessIterator(this, size));}


Result Vector::add(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] + val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for add(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] + vec->data[i];
    return std::move(Result(result));
}

Result Vector::sub(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] - val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for sub(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] - vec->data[i];
    return std::move(Result(result));
}

Result Vector::mul(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] * val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for mul(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] * vec->data[i];
    return std::move(Result(result));
}

Result Vector::div(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] / val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for div(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] / vec->data[i];
    return std::move(Result(result));
}

Result Vector::pow(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = std::pow(data[i], val);
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for pow(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = std::pow(data[i], vec->data[i]);
    return std::move(Result(result));
}

Result Vector::lt(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] < val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for lt(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] < vec->data[i];
    return std::move(Result(result));
}

Result Vector::gt(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] > val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for gt(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] > vec->data[i];
    return std::move(Result(result));
}

Result Vector::le(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] <= val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for le(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] <= vec->data[i];
    return std::move(Result(result));
}

Result Vector::ge(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] >= val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for ge(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] >= vec->data[i];
    return std::move(Result(result));
}

Result Vector::eq(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] == val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for eq(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] == vec->data[i];
    return std::move(Result(result));
}

Result Vector::neq(BMemory* memory, const DataPtr& other) {
    if(other.isfloat() || other.isint()) {
        double val = other.isfloat()?other.unsafe_tofloat():other.unsafe_toint();
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        Vector* result = new Vector(size);
        for (int i = 0; i < size; ++i) result->data[i] = data[i] != val;
        return std::move(Result(result));
    }
    bbassert(other.existsAndTypeEquals(VECTOR), "No builtin implementation for neq(vector, "+other.torepr()+")");
    Vector* vec = static_cast<Vector*>(other.get());
    std::lock_guard<std::recursive_mutex> lock1(memoryLock);
    std::lock_guard<std::recursive_mutex> lock2(vec->memoryLock);
    Vector* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = data[i] != vec->data[i];
    return std::move(Result(result));
}

Result Vector::sum(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    double total = 0;
    for (int i = 0; i < size; ++i) total += data[i];
    return Result(total);
}

Result Vector::min(BMemory* memory) {
    if (size == 0) return Result(OUT_OF_RANGE);
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    double minValue = data[0];
    for (int i = 1; i < size; ++i) if (data[i] < minValue) minValue = data[i];
    return Result(minValue);
}

Result Vector::max(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (size == 0) return Result(OUT_OF_RANGE);
    double maxValue = data[0];
    for (int i = 1; i < size; ++i) if (data[i] > maxValue) maxValue = data[i];
    return Result(maxValue);
}

Result Vector::logarithm(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    auto* result = new Vector(size);
    for (int i = 0; i < size; ++i) result->data[i] = std::log(data[i]);
    return Result(result);
}
