#ifndef BHASHMAP_H
#define BHASHMAP_H

#include "Data.h"
#include <memory>
#include <mutex>
#include "tsl/hopscotch_map.h"

class BHashMap : public Data {
public:
    BHashMap();
    virtual ~BHashMap();
    std::string toString(BMemory* memory) override;
    void put(Data* from, Data* to);
    Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory) override;

private:
    mutable std::recursive_mutex memoryLock;
    std::unordered_map<size_t, std::vector<std::pair<Data*, Data*>>> contents;
    friend class MapIterator; 
};
#endif // BHASHMAP_H
