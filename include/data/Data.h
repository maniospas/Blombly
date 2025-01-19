#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <atomic>
#include "common.h"
#include "Result.h"

class Data;
class BMemory;
struct BuiltinArgs {
    DataPtr arg0;
    DataPtr arg1;
    DataPtr arg2;
    uint8_t size;
    //DataPtr preallocResult;
};

// Abstract base class for all data types
class Data {
public:
    inline Datatype getType() const { return type; }

    Data(Datatype type);
    virtual ~Data() = default;

    static Result run(const OperationType operation, BuiltinArgs* args, BMemory* memory);
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory);
    virtual size_t toHash() const;
    virtual bool isSame(const DataPtr& other);

    virtual Result push(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result pop(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result next(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result at(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result put(BMemory* memory, const DataPtr& position, const DataPtr& value) {bberror("Internal error: No implementation for the given data objects");}
    virtual void clear(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result move(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual double toFloat(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual bool toBool(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual int64_t toInt(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result add(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result sub(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result mul(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result div(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result pow(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result mmul(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result mod(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result lt(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result le(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result gt(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result ge(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result eq(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result neq(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result opand(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result opor(BMemory* memory, const DataPtr& other) {bberror("Internal error: No implementation for the given data objects");}
    virtual int64_t len(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result iter(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result min(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result max(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result sum(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}
    virtual Result opnot(BMemory* memory) {bberror("Internal error: No implementation for the given data objects");}

    virtual std::string toString(BMemory* memory)= 0;
    
    inline void addOwner() {++referenceCounter;}
    virtual void removeFromOwner() {if((--referenceCounter)==0) delete this;}
protected:
    std::atomic<int> referenceCounter;
private:
    Datatype type;
};

bool DataPtr::existsAndTypeEquals(Datatype type) const {
    if(datatype & IS_NOT_PTR) return false;
    if(!data) return false;
    return std::bit_cast<Data*>(data)->getType() == type;
}

void DataPtr::existsAddOwner() const {
    if(datatype & IS_NOT_PTR) return;
    if(!data) return;
    std::bit_cast<Data*>(data)->addOwner();
}

void DataPtr::existsRemoveFromOwner() const {
    if(datatype & IS_NOT_PTR) return;
    if(!data) return;
    std::bit_cast<Data*>(data)->removeFromOwner();
}

#endif // DATA_H
