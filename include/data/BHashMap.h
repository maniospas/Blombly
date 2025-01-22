#ifndef BHASHMAP_H
#define BHASHMAP_H

#include "Data.h"
#include "Iterator.h"
#include <memory>
#include <mutex>
#include "tsl/hopscotch_map.h"

class BHashMap : public Data {
public:
    BHashMap();
    virtual ~BHashMap();
    std::string toString(BMemory* memory) override;
    Result put(BMemory* memory, const DataPtr& from, const DataPtr& to) override;
    Result at(BMemory* memory, const DataPtr& other) override;
    void clear(BMemory* memory) override;
    int64_t len(BMemory* memory) override;
    Result move(BMemory* memory) override;
    Result iter(BMemory* memory) override;
    void fastUnsafePut(const DataPtr& from, const DataPtr& to);
    //Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;

private:
    mutable std::recursive_mutex memoryLock;
    std::unordered_map<size_t, std::vector<std::pair<DataPtr, DataPtr>>> contents;
    friend class MapIterator; 
};


class MapIterator : public Iterator {
private:
    BHashMap* map;
    std::unordered_map<size_t, std::vector<std::pair<DataPtr, DataPtr>>>::iterator bucketIt;
    int64_t elementIndex;

public:
    MapIterator(BHashMap* map_);
    ~MapIterator() override = default;
    Result next(BMemory* memory) override;
    Result iter(BMemory* memory) override;
};

#endif // BHASHMAP_H
