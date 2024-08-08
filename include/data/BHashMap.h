// data/HashMap.h
#ifndef HASHMAP_H
#define HASHMAP_H

#include <memory>
#include <unordered_map>
#include <string>
#include "data/Data.h"

// Thread-safe container for hashmap contents
class HashMapContents {
private:
    pthread_mutex_t memoryLock;

public:
    std::unordered_map<std::string, Data*> contents;
    int lockable;

    HashMapContents();
    ~HashMapContents();
    void lock();
    void unlock();
    void unsafeUnlock();
};

// HashMap class representing a hashmap of data items
class BHashMap : public Data {
private:

public:
    std::shared_ptr<HashMapContents> contents;
    explicit BHashMap();
    explicit BHashMap(const std::shared_ptr<HashMapContents>& cont);
    ~BHashMap();

    int getType() const override;
    std::string toString() const override;
    Data* shallowCopy() const override;
    Data* implement(const OperationType operation, BuiltinArgs* args) override;
    void put(Data* from, Data* to);
};

#endif // HASHMAP_H
