#ifndef ITERATOR_H
#define ITERATOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "data/Data.h"
#include "data/Integer.h"

// Iterator class representing an iterator over a data object
class Iterator : public Data {
private:
    mutable std::recursive_mutex memoryLock;  // Use std::recursive_mutex for locking
    int size;
    Data* object;
    Integer* pos;

public:
    explicit Iterator(Data* object_);
    ~Iterator();

    std::string toString() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // ITERATOR_H
