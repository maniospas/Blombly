#ifndef BHASHMAP_H
#define BHASHMAP_H

#include "Data.h"
#include <memory>
#include <mutex>
#include "tsl/hopscotch_map.h"

// BHashMap class representing a hashmap data type
class BHashMap : public Data {
private:
    tsl::hopscotch_map<size_t, std::shared_ptr<Data>> contents;
    mutable std::recursive_mutex memoryLock;  // mutable to allow locking in const methods

public:
    explicit BHashMap();
    ~BHashMap();

    int getType() const override;
    std::string toString() const override;
    size_t toHash() const override { return 0; }  // Implement if necessary
    std::shared_ptr<Data> shallowCopy() const override;
    void put(const std::shared_ptr<Data>& from, const std::shared_ptr<Data>& to);

    std::shared_ptr<Data> implement(const OperationType operation, BuiltinArgs* args) override;
};

#endif // BHASHMAP_H
