#ifndef ITERATOR_H
#define ITERATOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"
#include "data/Integer.h"


class Iterator : public Data {
public:
    explicit Iterator();
    std::string toString()override;
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
    int first, last, step;
public:
    explicit IntRange(int first, int last, int step);
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
};

class FloatRange : public Iterator {
private:
    double first, last, step;
public:
    explicit FloatRange(double first, double last, double step);
    virtual Result implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // ITERATOR_H
