#ifndef DATA_H
#define DATA_H

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "common.h"

class Data;
struct BuiltinArgs {
    Data* arg0;
    Data* arg1;
    Data* arg2;
    int size;
    Data* preallocResult;
};

// Abstract base class for all data types
class Data {
public:
    bool isMutable = true;
    bool isDestroyable = true;

    virtual std::string toString() const = 0;
    virtual int getType() const = 0;
    virtual Data* shallowCopy() const = 0;
    virtual bool couldBeShallowCopy(Data* data) {
        return false;
    }
    virtual ~Data() = default;
    static Data* run(const OperationType operation, BuiltinArgs* args);
    virtual Data* implement(const OperationType operation, BuiltinArgs* args) {
        throw Unimplemented();
    }
    Data* shallowCopyIfNeeded() const {
        if(isDestroyable)
            return shallowCopy();
        return (Data*)this;
    }
};

#endif // DATA_H
