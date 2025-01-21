#ifndef STRUCT_H
#define STRUCT_H

#include <memory>
#include <string>
#include <mutex>
#include "data/Data.h"
#include "BMemory.h"

class Struct : public Data {
private:
    BMemory* memory;
    Result simpleImplement(int implementationCode, BMemory* scopeMemory);
    Result simpleImplement(int implementationCode, BMemory* scopeMemory, const DataPtr& other);

public:
    mutable std::recursive_mutex memoryLock;
    explicit Struct(BMemory* mem);
    ~Struct();
    BMemory* getMemory() const;
    void removeFromOwner() override;

    Result push(BMemory* scopeMemory, const DataPtr& other);
    Result pop(BMemory* scopeMemory) override;
    Result next(BMemory* scopeMemory) override;
    Result at(BMemory* scopeMemory, const DataPtr& other);
    Result put(BMemory* scopeMemory, const DataPtr& position, const DataPtr& value);
    void clear(BMemory* scopeMemory) override;
    int64_t len(BMemory* scopeMemory) override;
    Result move(BMemory* scopeMemory) override;
    Result iter(BMemory* scopeMemory) override;
    double toFloat(BMemory* scopeMemory) override;
    std::string toString(BMemory* calledMemory) override;
    bool toBool(BMemory* scopeMemory) override;
    int64_t toInt(BMemory* scopeMemory) override;
    Result add(BMemory* scopeMemory, const DataPtr& other) override;
    Result sub(BMemory* scopeMemory, const DataPtr& other) override;
    Result mul(BMemory* scopeMemory, const DataPtr& other) override;
    Result div(BMemory* scopeMemory, const DataPtr& other) override;
    Result pow(BMemory* scopeMemory, const DataPtr& other) override;
    Result mmul(BMemory* scopeMemory, const DataPtr& other) override;
    Result mod(BMemory* scopeMemory, const DataPtr& other) override;
    Result lt(BMemory* scopeMemory, const DataPtr& other) override;
    Result le(BMemory* scopeMemory, const DataPtr& other) override;
    Result gt(BMemory* scopeMemory, const DataPtr& other) override;
    Result ge(BMemory* scopeMemory, const DataPtr& other) override;
    Result eq(BMemory* scopeMemory, const DataPtr& other) override;
    Result neq(BMemory* scopeMemory, const DataPtr& other) override;
    Result opand(BMemory* scopeMemory, const DataPtr& other) override ;
    Result opor(BMemory* scopeMemory, const DataPtr& other) override;
    Result min(BMemory* scopeMemory) override;
    Result max(BMemory* scopeMemory) override;
    Result logarithm(BMemory* scopeMemory) override;
    Result sum(BMemory* scopeMemory) override;
    Result opnot(BMemory* scopeMemory) override;
};

#endif // STRUCT_H
