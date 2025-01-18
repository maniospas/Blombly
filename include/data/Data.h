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
    virtual std::string toString(BMemory* memory)= 0;
    inline Datatype getType() const { return type; }

    Data(Datatype type);
    virtual ~Data() = default;

    static Result run(const OperationType operation, BuiltinArgs* args, BMemory* memory);
    virtual Result implement(const OperationType operation, BuiltinArgs* args, BMemory* memory);
    virtual size_t toHash() const;
    virtual bool isSame(DataPtr other);
    
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
