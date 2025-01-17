#ifndef ITERATOR_H
#define ITERATOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"
#include "data/Integer.h"
#include "data/BFloat.h"


class Iterator : public Data {
public:
    explicit Iterator();
    virtual ~Iterator() {}
    std::string toString(BMemory* memory) override;
    virtual int64_t expectedSize() const {
        return 0;
    }
    virtual bool isContiguous() const {
        return false;
    }
    virtual int64_t getStart() const {
        bberror("Internal error: thechosen iterator type does not implement `getStart`, which means that `isContiguous` was not checked first.");
        return 0;
    }
    virtual int64_t getEnd() const {
        bberror("Internal error: the chosen iterator type does not implement `getEnd`, which means that `isContiguous` was not checked first.");
        return 0;
    }
    virtual DataPtr fastNext() {return nullptr;} // signify to JIT using this that it needs to fallback to implement to guarantee memory safety
};


class AccessIterator : public Iterator {
private:
    mutable std::recursive_mutex memoryLock;
    int64_t size;
    DataPtr object;
    Integer* pos;

public:
    explicit AccessIterator(DataPtr object_);
    ~AccessIterator();
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
};


class IntRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    int64_t first, last, step;
public:
    explicit IntRange(int64_t first, int64_t last, int64_t step);
    ~IntRange();
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
    int64_t expectedSize() const override {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        return (last-first)/step;
    }
    bool isContiguous() const override {return step==1;}
    int64_t getStart() const override {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        return first;
    }
    int64_t getEnd() const override {return last;}
    virtual DataPtr fastNext() override;
};

class FloatRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    double first, last, step;
public:
    explicit FloatRange(double first, double last, double step);
    ~FloatRange();
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;
    virtual DataPtr fastNext() override;
};

#endif // ITERATOR_H
