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
    std::shared_ptr<Data> object;
    std::shared_ptr<Integer> pos;

public:
    explicit Iterator(const std::shared_ptr<Data>& object_);
    ~Iterator();

    int getType() const override;
    std::string toString() const override;
    std::shared_ptr<Data> shallowCopy() const override;
    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // ITERATOR_H
