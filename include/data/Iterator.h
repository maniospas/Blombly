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
    std::string toString() override;
    virtual int expectedSize() const {
        return 0;
    }
    virtual bool isContiguous() const {
        return false;
    }
    virtual int getStart() const {
        bberror("Internal error: thechosen iterator type does not implement `getStart`, which means that `isContiguous` was not checked first.");
        return 0;
    }
    virtual int getEnd() const {
        bberror("Internal error: the chosen iterator type does not implement `getEnd`, which means that `isContiguous` was not checked first.");
        return 0;
    }
};


class AccessIterator : public Iterator {
private:
    mutable std::recursive_mutex memoryLock;
    int size;
    Data* object;
    Integer* pos;

public:
    explicit AccessIterator(Data* object_);
    ~AccessIterator();
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
};


class IntRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    int first, last, step;
public:
    explicit IntRange(int first, int last, int step);
    ~IntRange();
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
    int expectedSize() const override {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        return (last-first)/step;
    }
    bool isContiguous() const override {
        return step==1;
    }
    int getStart() const override {
        std::lock_guard<std::recursive_mutex> lock(memoryLock);
        return first;
    }
    int getEnd() const override {
        return last;
    }
};

class FloatRange : public Iterator {
private:
    mutable std::recursive_mutex memoryLock; 
    double first, last, step;
public:
    explicit FloatRange(double first, double last, double step);
    ~FloatRange();
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // ITERATOR_H
