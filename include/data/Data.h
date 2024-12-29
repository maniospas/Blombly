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
    Data* arg0;
    Data* arg1;
    Data* arg2;
    uint8_t size;
    //Data* preallocResult;
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
    virtual bool isSame(Data* other);
    
    virtual void addOwner();
    virtual void removeFromOwner();
    void leak();
    std::atomic<int> referenceCounter;
private:
    Datatype type;
};

#endif // DATA_H
