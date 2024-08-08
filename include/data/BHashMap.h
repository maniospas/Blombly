// data/BHashMap.h
#ifndef BHASHMAP_H
#define BHASHMAP_H

#include "Data.h"
#include <memory>
#include <pthread.h>
#include "tsl/hopscotch_map.h"
#include "tsl/hopscotch_set.h"

class HashMapContents {
public:
    tsl::hopscotch_map<size_t, Data*> contents;
    pthread_mutex_t memoryLock;
    int lockable;

    HashMapContents();
    ~HashMapContents();

    void lock();
    void unlock();
    void unsafeUnlock();
};

class BHashMap : public Data {
public:
    std::shared_ptr<HashMapContents> contents;

    BHashMap();
    BHashMap(const std::shared_ptr<HashMapContents>& cont);
    ~BHashMap();

    int getType() const override;
    std::string toString() const override;
    size_t toHash() const override { return 0; } // Implement if necessary
    Data* shallowCopy() const override;
    void put(Data* from, Data* to);

    Data* implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // BHASHMAP_H
