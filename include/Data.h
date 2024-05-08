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
    std::shared_ptr<Data> preallocResult;
};

// Abstract base class for all data types
class Data {
public:
    bool isMutable = true;

    virtual std::string toString() const = 0;
    virtual int getType() const = 0;
    virtual std::shared_ptr<Data> shallowCopy() const = 0;
    virtual bool couldBeShallowCopy(std::shared_ptr<Data> data) {
        return false;
    }
    virtual ~Data() = default;
    static std::shared_ptr<Data> run(const OperationType operation, const BuiltinArgs* args);
    virtual std::shared_ptr<Data> implement(const OperationType operation, const BuiltinArgs* args) {
        throw Unimplemented();
    }
};

#endif // DATA_H
