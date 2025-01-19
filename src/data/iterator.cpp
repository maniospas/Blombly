#include "data/Iterator.h"
#include "data/Integer.h"
#include "data/Data.h"
#include "data/BFloat.h"
#include "data/BError.h"
#include "common.h"
#include <iostream>
#include <mutex>

extern BError* OUT_OF_RANGE;
Iterator::Iterator() : Data(ITERATOR) {}
std::string Iterator::toString(BMemory* memory) {return "iterator";}

AccessIterator::AccessIterator(DataPtr object_, int64_t size) : object(object_.get()), pos(-1), size(size), Iterator() {}
AccessIterator::~AccessIterator() {}
Result AccessIterator::next(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    pos += 1; 
    if (pos >= size) return std::move(Result(OUT_OF_RANGE));
    return std::move(object->at(memory, pos));
}

IntRange::IntRange(int64_t first, int64_t last, int64_t step) : first(first), last(last), step(step), Iterator() {bbassert(step, "A range iterator with zero step never ends.");}
IntRange::~IntRange() {}
Result IntRange::next(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (step>0 && first >= last) return std::move(Result(OUT_OF_RANGE));
    if (step<0 && first <= last) return std::move(Result(OUT_OF_RANGE));
    DataPtr res(first);
    first += step;
    return std::move(Result(std::move(res)));
}
DataPtr IntRange::fastNext() {
    if (step>0 && first >= last) return OUT_OF_RANGE;
    if (step<0 && first <= last) return OUT_OF_RANGE;
    DataPtr res(first);
    first += step;
    return std::move(res);
}

FloatRange::FloatRange(double first, double last, double step) : first(first), last(last), step(step), Iterator() {bbassert(step, "A range iterator with zero step never ends.");}
FloatRange::~FloatRange() {}
Result FloatRange::next(BMemory* memory) {
    std::lock_guard<std::recursive_mutex> lock(memoryLock);
    if (step>0 && first >= last) return std::move(Result(OUT_OF_RANGE));
    if (step<0 && first <= last) return std::move(Result(OUT_OF_RANGE));
    DataPtr res(first);
    first += step;
    return std::move(Result(std::move(res)));
}
DataPtr FloatRange::fastNext() {
    if (step>0 && first >= last) return OUT_OF_RANGE;
    if (step<0 && first <= last) return OUT_OF_RANGE;
    DataPtr res(first);
    first += step;
    return std::move(res);
}