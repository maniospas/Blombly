// Iterator.h
#ifndef ITERATOR_H
#define ITERATOR_H

#include <memory>
#include <vector>
#include "Data.h"
#include "Integer.h"

// Thread-safe container for list contents
class IteratorContents {
private:
    pthread_mutex_t memoryLock;

public:
    int size;
    std::shared_ptr<Data> object;
    std::shared_ptr<Integer> pos;
    
    IteratorContents(const std::shared_ptr<Data>& object);
    void lock();
    void unlock();
};

// List class representing a list of data items
class Iterator : public Data {
private:
    std::shared_ptr<IteratorContents> contents;

public:
    explicit Iterator(const std::shared_ptr<IteratorContents>& cont);

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // ITERATOR_H
